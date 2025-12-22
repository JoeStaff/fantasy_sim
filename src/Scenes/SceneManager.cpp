#include <iostream>

#include "Scenes/SceneManager.h"
#include "Scenes/SceneFrameGrid.h"
#include "Platform/IVideo.h"

namespace Game {

SceneManager::SceneManager()
    : initialized_(false)
    , exit_requested_(false)
    , platform_manager_(nullptr)
    , focused_frame_(nullptr)
{
}

SceneManager::~SceneManager() {
    Shutdown();
}

bool SceneManager::Initialize(Platform::PlatformManager* platform_manager) {
    if (initialized_) {
        return true;
    }
    
    if (!platform_manager) {
        std::cerr << "SceneManager: Platform manager is null" << std::endl;
        return false;
    }
    
    platform_manager_ = platform_manager;
    initialized_ = true;
    
    return true;
}

void SceneManager::Shutdown() {
    // Exit all scene frames
    for (auto& [name, frame] : frames_) {
        if (frame && frame->GetScene()) {
            frame->GetScene()->OnExit();
            frame->GetScene()->Shutdown();
        }
    }
    frames_.clear();
    
    // Clear all registered scenes
    scenes_.clear();
    focused_frame_ = nullptr;
    initialized_ = false;
    platform_manager_ = nullptr;
}

void SceneManager::RegisterScene(std::unique_ptr<Scene> scene) {
    if (!scene) {
        std::cerr << "SceneManager: Attempted to register null scene" << std::endl;
        return;
    }
    
    std::string name = scene->GetName();
    if (scenes_.find(name) != scenes_.end()) {
        std::cerr << "SceneManager: Scene '" << name << "' already registered" << std::endl;
        return;
    }
    
    // Set scene manager reference
    scene->SetSceneManager(this);
    
    // Initialize scene
    if (!scene->Initialize(platform_manager_)) {
        std::cerr << "SceneManager: Failed to initialize scene '" << name << "'" << std::endl;
        return;
    }
    
    scenes_[name] = std::move(scene);
}

bool SceneManager::AddSceneFrame(const std::string& scene_name, i32 x, i32 y, i32 width, i32 height) {
    Scene* scene = FindScene(scene_name);
    if (!scene) {
        std::cerr << "SceneManager: Scene '" << scene_name << "' not found" << std::endl;
        return false;
    }
    
    // Check if frame already exists
    if (frames_.find(scene_name) != frames_.end()) {
        std::cerr << "SceneManager: Scene frame for '" << scene_name << "' already exists" << std::endl;
        return false;
    }
    
    // If using grid layout, remove from grid first (this frame uses absolute positioning)
    if (grid_layout_) {
        grid_layout_->RemoveScene(scene_name);
    }
    
    // Create frame
    auto frame = std::make_unique<SceneFrame>(scene, x, y, width, height);
    
    // If this is the first frame, give it focus
    if (!focused_frame_) {
        frame->SetFocus(true);
        focused_frame_ = frame.get();
        scene->OnEnter();
    } else {
        frame->SetFocus(false);
    }
    
    frames_[scene_name] = std::move(frame);
    return true;
}

bool SceneManager::AddSceneFrameGrid(const std::string& scene_name, i32 grid_x, i32 grid_y,
                                     i32 grid_width, i32 grid_height) {
    Scene* scene = FindScene(scene_name);
    if (!scene) {
        std::cerr << "SceneManager: Scene '" << scene_name << "' not found" << std::endl;
        return false;
    }
    
    // Check if grid layout is set
    if (!grid_layout_) {
        std::cerr << "SceneManager: No grid layout set. Call SetGridLayout() first." << std::endl;
        return false;
    }
    
    // Add scene to grid
    if (!grid_layout_->AddScene(scene_name, grid_x, grid_y, grid_width, grid_height)) {
        std::cerr << "SceneManager: Failed to add scene '" << scene_name << "' to grid (overlap or out of bounds)" << std::endl;
        return false;
    }
    
    // Check if frame already exists, if so remove it (will recreate with grid bounds)
    bool frame_existed = (frames_.find(scene_name) != frames_.end());
    if (frame_existed) {
        SceneFrame* old_frame = frames_[scene_name].get();
        if (old_frame == focused_frame_) {
            focused_frame_ = nullptr;
        }
        frames_.erase(scene_name);
    }
    
    // Calculate frame bounds from grid
    if (!platform_manager_ || !platform_manager_->GetVideo()) {
        std::cerr << "SceneManager: Cannot calculate grid bounds without video" << std::endl;
        return false;
    }
    
    auto* video = platform_manager_->GetVideo();
    i32 window_width = video->GetWindowWidth();
    i32 window_height = video->GetWindowHeight();
    
    i32 x, y, width, height;
    grid_layout_->CalculateFrameBounds(scene_name, window_width, window_height, x, y, width, height);
    
    // Create frame with calculated bounds
    auto frame = std::make_unique<SceneFrame>(scene, x, y, width, height);
    
    // If this is the first frame, give it focus
    if (!focused_frame_) {
        frame->SetFocus(true);
        focused_frame_ = frame.get();
        scene->OnEnter();
    } else {
        frame->SetFocus(false);
    }
    
    frames_[scene_name] = std::move(frame);
    return true;
}

void SceneManager::SetGridLayout(i32 grid_cols, i32 grid_rows) {
    grid_layout_ = std::make_unique<SceneFrameGrid>(grid_cols, grid_rows);
    
    // Update all existing frames that are in the grid
    if (platform_manager_ && platform_manager_->GetVideo()) {
        auto* video = platform_manager_->GetVideo();
        i32 window_width = video->GetWindowWidth();
        i32 window_height = video->GetWindowHeight();
        
        // Convert frames map to the format UpdateFrameBounds expects
        std::unordered_map<std::string, SceneFrame*> frame_ptrs;
        for (auto& [name, frame] : frames_) {
            frame_ptrs[name] = frame.get();
        }
        
        grid_layout_->UpdateFrameBounds(frame_ptrs, window_width, window_height);
    }
}

bool SceneManager::RemoveSceneFrame(const std::string& scene_name) {
    auto it = frames_.find(scene_name);
    if (it == frames_.end()) {
        return false;
    }
    
    SceneFrame* frame = it->second.get();
    if (frame && frame->GetScene()) {
        frame->GetScene()->OnExit();
    }
    
    // If this was the focused frame, clear focus
    if (focused_frame_ == frame) {
        focused_frame_ = nullptr;
        // Focus the first remaining frame if any
        for (auto& [name, f] : frames_) {
            if (f.get() != frame) {
                f->SetFocus(true);
                focused_frame_ = f.get();
                if (f->GetScene()) {
                    f->GetScene()->OnEnter();
                }
                break;
            }
        }
    }
    
    frames_.erase(it);
    return true;
}

SceneFrame* SceneManager::GetSceneFrame(const std::string& scene_name) {
    auto it = frames_.find(scene_name);
    if (it != frames_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool SceneManager::SetFocus(const std::string& scene_name) {
    SceneFrame* frame = GetSceneFrame(scene_name);
    if (!frame) {
        return false;
    }
    
    // Remove focus from current focused frame
    if (focused_frame_) {
        focused_frame_->SetFocus(false);
        if (focused_frame_->GetScene()) {
            focused_frame_->GetScene()->OnExit();
        }
    }
    
    // Set focus to new frame
    frame->SetFocus(true);
    focused_frame_ = frame;
    if (frame->GetScene()) {
        frame->GetScene()->OnEnter();
    }
    
    return true;
}

SceneFrame* SceneManager::GetFocusedFrame() const {
    return focused_frame_;
}

void SceneManager::Update(f32 delta_time) {
    // Update all visible scene frames
    for (auto& [name, frame] : frames_) {
        if (frame && frame->IsVisible() && frame->GetScene()) {
            frame->GetScene()->Update(delta_time);
        }
    }
}

void SceneManager::Render() {
    if (!platform_manager_ || !platform_manager_->GetVideo()) {
        return;
    }
    
    auto* video = platform_manager_->GetVideo();
    
    // Update grid layout if window size changed
    static i32 last_window_width = 0;
    static i32 last_window_height = 0;
    i32 current_width = video->GetWindowWidth();
    i32 current_height = video->GetWindowHeight();
    if ((last_window_width != current_width || last_window_height != current_height) && grid_layout_) {
        OnWindowResized(current_width, current_height);
        last_window_width = current_width;
        last_window_height = current_height;
    }
    
    // Clear screen
    video->Clear(30, 30, 30, 255);
    
    // Render all visible frames (order doesn't matter much for rendering, but we'll render in insertion order)
    for (auto& [name, frame] : frames_) {
        if (frame && frame->IsVisible()) {
            RenderFrame(frame.get());
        }
    }
}

void SceneManager::RenderFrame(SceneFrame* frame) {
    if (!frame || !platform_manager_ || !platform_manager_->GetVideo()) {
        return;
    }
    
    auto* video = platform_manager_->GetVideo();
    Scene* scene = frame->GetScene();
    if (!scene) {
        return;
    }
    
    // Save current viewport
    video->ResetViewport();
    
    // Set viewport to frame bounds
    video->SetViewport(frame->GetX(), frame->GetY(), frame->GetWidth(), frame->GetHeight());
    
    // Render the scene
    scene->Render(video);
    
    // Reset viewport
    video->ResetViewport();
    
    // Draw border if out of focus (opaque gray border)
    if (!frame->HasFocus()) {
        i32 border_width = 4;
        video->SetDrawColor(128, 128, 128, 255);  // Opaque gray
        
        // Draw border rectangles
        video->DrawRect(frame->GetX(), frame->GetY(), frame->GetWidth(), border_width);  // Top
        video->DrawRect(frame->GetX(), frame->GetY() + frame->GetHeight() - border_width, frame->GetWidth(), border_width);  // Bottom
        video->DrawRect(frame->GetX(), frame->GetY(), border_width, frame->GetHeight());  // Left
        video->DrawRect(frame->GetX() + frame->GetWidth() - border_width, frame->GetY(), border_width, frame->GetHeight());  // Right
    }
}

void SceneManager::ProcessInput() {
    if (!platform_manager_ || !platform_manager_->GetInput()) {
        return;
    }
    
    auto* input = platform_manager_->GetInput();
    
    // Handle click-to-focus
    if (input->IsMouseButtonPressed(Platform::MouseButton::Left)) {
        i32 mouse_x, mouse_y;
        input->GetMousePosition(mouse_x, mouse_y);
        
        // Find frame at mouse position (last one found wins for layering)
        SceneFrame* clicked_frame = nullptr;
        for (auto& [name, frame] : frames_) {
            if (frame && frame->IsVisible() && frame->ContainsPoint(mouse_x, mouse_y)) {
                clicked_frame = frame.get();
            }
        }
        
        // If clicked on a different frame, switch focus
        if (clicked_frame && clicked_frame != focused_frame_) {
            SetFocus(clicked_frame->GetScene()->GetName());
            // Don't process input this frame for the newly focused scene
            return;
        }
    }
    
    // Process input only for focused frame
    if (focused_frame_ && focused_frame_->GetScene()) {
        // Convert mouse coordinates to frame-local coordinates
        i32 mouse_x, mouse_y;
        input->GetMousePosition(mouse_x, mouse_y);
        
        // Check if mouse is within focused frame bounds
        if (focused_frame_->ContainsPoint(mouse_x, mouse_y)) {
            // Mouse coordinates are in screen space, but scenes expect frame-local coordinates
            // Since we set the viewport to frame bounds, drawing is relative to frame
            // But mouse coordinates are still in screen space, so scenes need to account for this
            // For now, pass input directly - scenes that need frame-local coords can adjust
            // TODO: Create an input wrapper that transforms coordinates to frame-local space
            focused_frame_->GetScene()->ProcessInput(input);
        }
    }
}

// Legacy compatibility methods
bool SceneManager::ChangeScene(const std::string& scene_name) {
    // Remove all existing frames
    frames_.clear();
    focused_frame_ = nullptr;
    
    // Add new scene as fullscreen frame
    if (!platform_manager_ || !platform_manager_->GetVideo()) {
        return false;
    }
    
    auto* video = platform_manager_->GetVideo();
    i32 width = video->GetWindowWidth();
    i32 height = video->GetWindowHeight();
    
    return AddSceneFrame(scene_name, 0, 0, width, height);
}

bool SceneManager::PushScene(const std::string& scene_name) {
    // For compatibility, just add as a fullscreen frame (will cover existing frames)
    if (!platform_manager_ || !platform_manager_->GetVideo()) {
        return false;
    }
    
    auto* video = platform_manager_->GetVideo();
    i32 width = video->GetWindowWidth();
    i32 height = video->GetWindowHeight();
    
    // Remove existing frame for this scene if it exists
    RemoveSceneFrame(scene_name);
    
    return AddSceneFrame(scene_name, 0, 0, width, height);
}

bool SceneManager::PopScene() {
    // Remove the focused frame
    if (focused_frame_ && focused_frame_->GetScene()) {
        return RemoveSceneFrame(focused_frame_->GetScene()->GetName());
    }
    return false;
}

Scene* SceneManager::GetCurrentScene() const {
    if (focused_frame_) {
        return focused_frame_->GetScene();
    }
    return nullptr;
}

SceneFrame* SceneManager::GetFrameAtPoint(i32 x, i32 y) const {
    // Check frames (last one found wins for layering)
    SceneFrame* found = nullptr;
    for (auto& [name, frame] : frames_) {
        if (frame && frame->IsVisible() && frame->ContainsPoint(x, y)) {
            found = frame.get();
        }
    }
    return found;
}

void SceneManager::OnWindowResized(i32 new_width, i32 new_height) {
    // Update all frames in grid layout
    if (grid_layout_) {
        std::unordered_map<std::string, SceneFrame*> frame_ptrs;
        for (auto& [name, frame] : frames_) {
            frame_ptrs[name] = frame.get();
        }
        grid_layout_->UpdateFrameBounds(frame_ptrs, new_width, new_height);
    }
}

Scene* SceneManager::FindScene(const std::string& scene_name) const {
    auto it = scenes_.find(scene_name);
    if (it != scenes_.end()) {
        return it->second.get();
    }
    return nullptr;
}

} // namespace Game
