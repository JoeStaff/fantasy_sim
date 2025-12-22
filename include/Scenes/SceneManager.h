#pragma once

#include "Core/Types.h"
#include "Platform/PlatformManager.h"
#include "Scenes/Scene.h"
#include "Scenes/SceneFrame.h"
#include "Scenes/SceneFrameGrid.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Game {

// Manages scene frames - scenes as positioned frames on screen
class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    
    // Initialize with platform manager
    bool Initialize(Platform::PlatformManager* platform_manager);
    
    // Shutdown and cleanup
    void Shutdown();
    
    // Register a scene (takes ownership)
    void RegisterScene(std::unique_ptr<Scene> scene);
    
    // Add a scene frame to the screen
    // Returns true on success, false if scene not found or already has a frame
    bool AddSceneFrame(const std::string& scene_name, i32 x, i32 y, i32 width, i32 height);
    
    // Add a scene frame using grid layout
    // grid_x, grid_y: Grid cell position (0-based)
    // grid_width, grid_height: Number of cells to span
    // Returns true on success
    bool AddSceneFrameGrid(const std::string& scene_name, i32 grid_x, i32 grid_y, 
                           i32 grid_width, i32 grid_height);
    
    // Set up a grid layout for automatic frame positioning
    // This will clear any existing grid layout
    void SetGridLayout(i32 grid_cols, i32 grid_rows);
    
    // Get the grid layout (returns nullptr if no grid is set)
    SceneFrameGrid* GetGridLayout() { return grid_layout_.get(); }
    
    // Remove a scene frame by scene name
    bool RemoveSceneFrame(const std::string& scene_name);
    
    // Get scene frame by scene name
    SceneFrame* GetSceneFrame(const std::string& scene_name);
    
    // Change focus to a frame by scene name
    bool SetFocus(const std::string& scene_name);
    
    // Get the currently focused frame
    SceneFrame* GetFocusedFrame() const;
    
    // Update all visible scenes
    void Update(f32 delta_time);
    
    // Render all visible scene frames
    void Render();
    
    // Process input - only passes to focused frame, handles click-to-focus
    void ProcessInput();
    
    // Update frame bounds when window is resized (called automatically, but can be called manually)
    void OnWindowResized(i32 new_width, i32 new_height);
    
    // Check if manager is initialized
    bool IsInitialized() const { return initialized_; }
    
    // Request game exit (called by scenes like QuitMenu)
    void RequestExit() { exit_requested_ = true; }
    
    // Check if exit was requested
    bool IsExitRequested() const { return exit_requested_; }
    
    // Legacy compatibility methods (deprecated - use AddSceneFrame instead)
    bool ChangeScene(const std::string& scene_name);
    bool PushScene(const std::string& scene_name);
    bool PopScene();
    Scene* GetCurrentScene() const;
    
private:
    bool initialized_ = false;
    bool exit_requested_ = false;
    Platform::PlatformManager* platform_manager_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes_;
    std::unordered_map<std::string, std::unique_ptr<SceneFrame>> frames_;  // Key is scene name
    SceneFrame* focused_frame_ = nullptr;
    std::unique_ptr<SceneFrameGrid> grid_layout_;
    
    // Helper to find scene
    Scene* FindScene(const std::string& scene_name) const;
    
    // Helper to get frame at point (for click-to-focus)
    SceneFrame* GetFrameAtPoint(i32 x, i32 y) const;
    
    // Render a frame with border if out of focus
    void RenderFrame(SceneFrame* frame);
};

} // namespace Game
