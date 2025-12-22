#include "Scenes/WorldScene.h"
#include "Scenes/WorldSidebarScene.h"
#include "Scenes/SceneManager.h"
#include "Simulation/Region.h"
#include "Simulation/SimulationManager.h"
#include "Simulation/World.h"
#include "Simulation/LODSystem.h"
#include "Core/Config.h"
#include "Platform/IInput.h"
#include "Platform/IVideo.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace Game {

WorldScene::WorldScene()
    : Scene("WorldScene")
{
    // Constructor initializes base Scene with name
}

bool WorldScene::Initialize(Platform::PlatformManager* platform_manager) {
    platform_manager_ = platform_manager;
    
    // Get config
    auto& config = Config::Configuration::GetInstance();
    grid_width_ = config.world.region_grid_width;
    grid_height_ = config.world.region_grid_height;
    region_size_ = 50.0f;  // Base pixel size
    
    // Initialize simulation manager
    simulation_manager_ = std::make_unique<Simulation::SimulationManager>();
    if (!simulation_manager_->Initialize()) {
        return false;
    }
    
    // Share simulation manager with sidebar scene
    WorldSceneSharedState::g_simulation_manager = simulation_manager_.get();
    
    // Center camera on world (do this before creating regions so debug output is correct)
    camera_x_ = (grid_width_ * region_size_) / 2.0f;
    camera_y_ = (grid_height_ * region_size_) / 2.0f;
    
    // Setup region grid (this triggers world generation which loads region definitions from JSON)
    if (simulation_manager_) {
        simulation_manager_->InitializeRegionGrid(grid_width_, grid_height_, region_size_);
        
        // Initialize region colors AFTER world generation (which loads region definitions from JSON)
        InitializeRegionColors();
        
        // Debug: Verify regions were created
        const auto& regions = simulation_manager_->GetRegions();
        std::cout << "WorldScene: Created " << regions.size() << " regions" << std::endl;
        std::cout << "WorldScene: Grid size " << grid_width_ << "x" << grid_height_ << std::endl;
        std::cout << "WorldScene: Region size " << region_size_ << std::endl;
        std::cout << "WorldScene: Camera at (" << camera_x_ << ", " << camera_y_ << ")" << std::endl;
    }
    
    // Debug: Start with a closer zoom to see regions better
    zoom_level_ = 0.5f;  // Start zoomed out to see more
    
    return true;
}

void WorldScene::Shutdown() {
    simulation_manager_.reset();
}

void WorldScene::Update(f32 delta_time) {
    // Input handling is done in ProcessInput, but we also handle continuous input here
    if (!platform_manager_ || !platform_manager_->GetInput()) {
        return;
    }
    
    auto* input = platform_manager_->GetInput();
    
    // Handle scrolling (continuous movement)
    HandleScrolling(delta_time, input);
    
    // Handle zooming (continuous zoom)
    HandleZooming(delta_time, input);
    
    // Update simulation
    if (simulation_manager_) {
        simulation_manager_->Update(delta_time);
    }
    
    // Update LOD when selection changes (called from HandleRegionSelection)
    // LOD is also updated when camera moves significantly
}

void WorldScene::Render(Platform::IVideo* video) {
    if (!video) {
        return;
    }
    
    // Clear screen with dark background
    video->Clear(20, 20, 30, 255);
    
    // Render regions (viewport is set by SceneManager to frame bounds)
    RenderRegions(video);
}

void WorldScene::ProcessInput(Platform::IInput* input) {
    if (!input) {
        return;
    }
    
    // Quick zoom: F5 zooms out fully
    if (input->IsKeyPressed(Platform::KeyCode::F5)) {
        zoom_level_ = MIN_ZOOM;
    }
    
    // Handle Escape key to return to main menu
    if (input->IsKeyPressed(Platform::KeyCode::Escape)) {
        if (scene_manager_) {
            scene_manager_->ChangeScene("MainMenu");
        }
        return;
    }
    
    // Handle region selection
    HandleRegionSelection(input);
}

void WorldScene::OnEnter() {
    // Reset camera to center when entering scene
    // camera_x_ = (grid_width_ * region_size_) / 2.0f;
    // camera_y_ = (grid_height_ * region_size_) / 2.0f;
    // zoom_level_ = 1.0f;
}

void WorldScene::OnExit() {
    // Cleanup if needed
}

void WorldScene::HandleScrolling(f32 delta_time, Platform::IInput* input) {
    if (!input) {
        return;
    }
    
    f32 scroll_delta = SCROLL_SPEED * delta_time;
    f32 dx = 0.0f;
    f32 dy = 0.0f;
    
    // WASD keys
    if (input->IsKeyDown(Platform::KeyCode::W) || input->IsKeyDown(Platform::KeyCode::Up)) {
        dy -= scroll_delta;
    }
    if (input->IsKeyDown(Platform::KeyCode::S) || input->IsKeyDown(Platform::KeyCode::Down)) {
        dy += scroll_delta;
    }
    if (input->IsKeyDown(Platform::KeyCode::A) || input->IsKeyDown(Platform::KeyCode::Left)) {
        dx -= scroll_delta;
    }
    if (input->IsKeyDown(Platform::KeyCode::D) || input->IsKeyDown(Platform::KeyCode::Right)) {
        dx += scroll_delta;
    }
    
    // Update camera position
    camera_x_ += dx;
    camera_y_ += dy;
    
    // Clamp camera to world bounds (optional - can allow infinite scrolling)
    f32 world_width = grid_width_ * region_size_;
    f32 world_height = grid_height_ * region_size_;
    camera_x_ = std::max(0.0f, std::min(camera_x_, world_width));
    camera_y_ = std::max(0.0f, std::min(camera_y_, world_height));
}

void WorldScene::HandleZooming(f32 delta_time, Platform::IInput* input) {
    if (!input) {
        return;
    }
    
    // Use Q/E keys for zoom
    if (input->IsKeyDown(Platform::KeyCode::Q)) {
        zoom_level_ -= ZOOM_SPEED * delta_time;
    }
    if (input->IsKeyDown(Platform::KeyCode::E)) {
        zoom_level_ += ZOOM_SPEED * delta_time;
    }
    
    // Mouse wheel (check every frame, but only process if changed)
    i32 wheel_x, wheel_y;
    input->GetMouseWheel(wheel_x, wheel_y);
    if (wheel_y != 0) {
        f32 zoom_delta = wheel_y > 0 ? ZOOM_SPEED * 0.5f : -ZOOM_SPEED * 0.5f;
        zoom_level_ += zoom_delta;
        std::cout << "WorldScene: Zoom changed to " << zoom_level_ << std::endl;
    }
    
    // Clamp zoom
    zoom_level_ = std::max(MIN_ZOOM, std::min(MAX_ZOOM, zoom_level_));
}

void WorldScene::HandleRegionSelection(Platform::IInput* input) {
    if (!input->IsMouseButtonPressed(Platform::MouseButton::Left)) {
        return;
    }
    
    i32 mouse_x, mouse_y;
    input->GetMousePosition(mouse_x, mouse_y);
    
    // Convert screen coordinates to frame-local coordinates
    i32 frame_x, frame_y, frame_width, frame_height;
    GetFrameBounds(frame_x, frame_y, frame_width, frame_height);
    mouse_x -= frame_x;
    mouse_y -= frame_y;
    
    RegionID region_id = GetRegionAtScreenPosition(mouse_x, mouse_y);
    if (region_id != INVALID_REGION_ID) {
        // Single selection - if clicking same region, deselect; otherwise select new one
        if (selected_region_id_ == region_id) {
            selected_region_id_ = INVALID_REGION_ID;
        } else {
            selected_region_id_ = region_id;
        }
        
        // Update shared state for sidebar scene
        WorldSceneSharedState::g_selected_region_id = selected_region_id_;
        
        // Update simulation LOD based on selection
        UpdateSimulationLOD();
    }
}

void WorldScene::RenderRegions(Platform::IVideo* video) {
    if (!simulation_manager_) {
        return;
    }
    
    const auto& regions = simulation_manager_->GetRegions();
    
    if (regions.empty()) {
        return;
    }
    
    // Get frame dimensions (viewport dimensions when rendered)
    i32 frame_x, frame_y, frame_width, frame_height;
    GetFrameBounds(frame_x, frame_y, frame_width, frame_height);
    
    f32 scaled_region_size = region_size_ * zoom_level_;
    
    // Calculate visible region bounds (in world coordinates)
    f32 view_left = camera_x_ - (frame_width / 2.0f) / zoom_level_;
    f32 view_right = camera_x_ + (frame_width / 2.0f) / zoom_level_;
    f32 view_top = camera_y_ - (frame_height / 2.0f) / zoom_level_;
    f32 view_bottom = camera_y_ + (frame_height / 2.0f) / zoom_level_;
    
    // Expand view bounds slightly to avoid edge cases
    view_left -= region_size_;
    view_right += region_size_;
    view_top -= region_size_;
    view_bottom += region_size_;
    
    // Render each region
    u32 rendered_count = 0;
    static bool debug_printed = false;
    for (const auto& region : regions) {
        if (!region) {
            continue;
        }
        
        f32 world_x = region->GetX();
        f32 world_y = region->GetY();
        
        // Check if region is visible (use world region_size_)
        if (world_x + region_size_ < view_left || world_x > view_right ||
            world_y + region_size_ < view_top || world_y > view_bottom) {
            continue;
        }
        
        // Convert to screen coordinates
        i32 screen_x, screen_y;
        WorldToScreen(world_x, world_y, screen_x, screen_y);
        
        i32 screen_size = static_cast<i32>(scaled_region_size);
        
        // Get frame dimensions for bounds checking (reuse variables from earlier)
        GetFrameBounds(frame_x, frame_y, frame_width, frame_height);
        
        // Only render if region is at least partially on screen (with some margin)
        if (screen_x + screen_size < -10 || screen_x > frame_width + 10 ||
            screen_y + screen_size < -10 || screen_y > frame_height + 10) {
            continue;
        }
        
        // Ensure minimum size for visibility
        if (screen_size < 1) {
            screen_size = 1;
        }
        
        // Render region
        RenderRegion(video, region.get(), screen_x, screen_y, screen_size);
        rendered_count++;
    }
    
    // Debug output (once)
    if (!debug_printed && rendered_count > 0) {
        std::cout << "WorldScene: Rendered " << rendered_count << " regions" << std::endl;
        std::cout << "WorldScene: View bounds: left=" << view_left << " right=" << view_right 
                  << " top=" << view_top << " bottom=" << view_bottom << std::endl;
        debug_printed = true;
    }
}

void WorldScene::RenderRegion(Platform::IVideo* video, Simulation::Region* region,
                              i32 screen_x, i32 screen_y, i32 screen_size) {
    if (!region || !video) {
        return;
    }
    
    // Get region type and color
    std::string region_type = GetRegionType(region);
    u8 r, g, b;
    GetRegionColor(region_type, r, g, b);
    
    // Check if selected
    bool is_selected = (region->GetID() == selected_region_id_);
    
    // Draw filled rectangle (semi-transparent) - alpha blending must be enabled
    video->SetDrawColor(r, g, b, 128);  // 50% opacity
    video->DrawRect(screen_x, screen_y, screen_size, screen_size);
    
    // Draw border (fully opaque) - drawn after fill so it's on top
    if (is_selected) {
        // Highlight selected regions with brighter border
        video->SetDrawColor(255, 255, 0, 255);  // Yellow for selected
    } else {
        video->SetDrawColor(r, g, b, 255);  // Fully opaque border (same color as fill)
    }
    video->DrawRectOutline(screen_x, screen_y, screen_size, screen_size);
}

void WorldScene::GetRegionColor(const std::string& region_type, u8& r, u8& g, u8& b) {
    auto it = region_colors_.find(region_type);
    if (it != region_colors_.end()) {
        std::tie(r, g, b) = it->second;
    } else {
        // Default gray color for unknown types
        r = g = b = 128;
    }
}

void WorldScene::WorldToScreen(f32 world_x, f32 world_y, i32& screen_x, i32& screen_y) {
    // Get frame dimensions (viewport dimensions when rendered)
    i32 frame_x, frame_y, frame_width, frame_height;
    GetFrameBounds(frame_x, frame_y, frame_width, frame_height);
    
    // Convert world coordinates to screen coordinates (relative to frame)
    // Center viewport
    f32 screen_fx = (world_x - camera_x_) * zoom_level_ + (frame_width / 2.0f);
    f32 screen_fy = (world_y - camera_y_) * zoom_level_ + (frame_height / 2.0f);
    
    screen_x = static_cast<i32>(screen_fx);
    screen_y = static_cast<i32>(screen_fy);
}

void WorldScene::ScreenToWorld(i32 screen_x, i32 screen_y, f32& world_x, f32& world_y) {
    // Get frame dimensions (viewport dimensions when rendered)
    i32 frame_x, frame_y, frame_width, frame_height;
    GetFrameBounds(frame_x, frame_y, frame_width, frame_height);
    
    // Convert screen coordinates (frame-local) to world coordinates
    // Center viewport
    world_x = (screen_x - (frame_width / 2.0f)) / zoom_level_ + camera_x_;
    world_y = (screen_y - (frame_height / 2.0f)) / zoom_level_ + camera_y_;
}

RegionID WorldScene::GetRegionAtScreenPosition(i32 screen_x, i32 screen_y) {
    if (!simulation_manager_) {
        return INVALID_REGION_ID;
    }
    
    // Convert screen to world coordinates
    f32 world_x, world_y;
    ScreenToWorld(screen_x, screen_y, world_x, world_y);
    
    // Find region at this position using world region_size_ (not scaled)
    const auto& regions = simulation_manager_->GetRegions();
    
    // Calculate which grid cell the point is in
    u16 grid_x = static_cast<u16>(world_x / region_size_);
    u16 grid_y = static_cast<u16>(world_y / region_size_);
    
    // Clamp to grid bounds
    if (grid_x >= grid_width_ || grid_y >= grid_height_) {
        return INVALID_REGION_ID;
    }
    
    // Calculate region index from grid position
    u32 region_index = static_cast<u32>(grid_y) * static_cast<u32>(grid_width_) + static_cast<u32>(grid_x);
    
    if (region_index < regions.size()) {
        return regions[region_index]->GetID();
    }
    
    return INVALID_REGION_ID;
}

void WorldScene::InitializeRegionColors() {
    // Clear existing colors to ensure fresh load from JSON
    region_colors_.clear();
    
    // Load colors from region definitions in config
    auto& config = Config::Configuration::GetInstance();
    const auto& region_definitions = config.regions.region_definitions;
    
    // Load colors from region definitions (JSON file)
    for (const auto& [type, def] : region_definitions) {
        region_colors_[type] = std::make_tuple(def.color_r, def.color_g, def.color_b);
    }
    
    // Only set fallback colors for types that are NOT in the JSON definitions
    // These are legacy fallbacks in case JSON is missing some types
    if (region_colors_.find("Forest") == region_colors_.end()) {
        region_colors_["Forest"] = std::make_tuple(34, 139, 34);      // Forest green
    }
    if (region_colors_.find("Water") == region_colors_.end()) {
        region_colors_["Water"] = std::make_tuple(30, 144, 255);      // Dodger blue
    }
    if (region_colors_.find("Coastal") == region_colors_.end()) {
        region_colors_["Coastal"] = std::make_tuple(30, 144, 255);    // Dodger blue
    }
    if (region_colors_.find("Desert") == region_colors_.end()) {
        region_colors_["Desert"] = std::make_tuple(238, 203, 173);    // Sandy brown
    }
    if (region_colors_.find("Plains") == region_colors_.end()) {
        region_colors_["Plains"] = std::make_tuple(144, 238, 144);   // Light green
    }
    if (region_colors_.find("Urban") == region_colors_.end()) {
        region_colors_["Urban"] = std::make_tuple(105, 105, 105);    // Dim gray
    }
    if (region_colors_.find("City") == region_colors_.end()) {
        region_colors_["City"] = std::make_tuple(105, 105, 105);     // Dim gray
    }
    if (region_colors_.find("Rural") == region_colors_.end()) {
        region_colors_["Rural"] = std::make_tuple(154, 205, 50);     // Yellow green
    }
    if (region_colors_.find("Mountain") == region_colors_.end()) {
        region_colors_["Mountain"] = std::make_tuple(139, 137, 137); // Dark gray
    }
    if (region_colors_.find("Road") == region_colors_.end()) {
        region_colors_["Road"] = std::make_tuple(160, 82, 45);       // Sienna
    }
    if (region_colors_.find("River") == region_colors_.end()) {
        region_colors_["River"] = std::make_tuple(70, 130, 180);      // Steel blue
    }
    if (region_colors_.find("RiverSource") == region_colors_.end()) {
        region_colors_["RiverSource"] = std::make_tuple(100, 150, 200); // Lighter blue for sources
    }
    if (region_colors_.find("Woods") == region_colors_.end()) {
        region_colors_["Woods"] = std::make_tuple(12, 12, 34);      // Dark olive green
    }
    
    // Debug output to verify colors were loaded
    std::cout << "WorldScene: Loaded " << region_colors_.size() << " region colors from JSON" << std::endl;
}

void WorldScene::InitializeRegions() {
    // Regions are initialized by SimulationManager::InitializeRegionGrid
}

std::string WorldScene::GetRegionType(Simulation::Region* region) {
    if (!region) {
        return "Unknown";
    }
    return region->GetType();
}

void WorldScene::UpdateSimulationLOD() {
    if (!simulation_manager_) {
        return;
    }
    
    auto& config = Config::Configuration::GetInstance();
    u8 neighbor_range = config.simulation.lod.neighbor_range;
    
    // Get all regions
    const auto& regions = simulation_manager_->GetRegions();
    
    // Build LOD assignment lists
    std::vector<RegionID> full_sim_regions;
    std::vector<RegionID> half_sim_regions;
    std::vector<RegionID> formula_sim_regions;
    
    if (selected_region_id_ != INVALID_REGION_ID) {
        // Get selected region and its neighbors for full simulation
        full_sim_regions = GetNeighborRegions(selected_region_id_, neighbor_range);
        full_sim_regions.push_back(selected_region_id_);  // Include selected region itself
    }
    
    // Assign LOD to all regions
    for (const auto& region : regions) {
        if (!region) {
            continue;
        }
        
        RegionID region_id = region->GetID();
        
        // Check if in full simulation set
        if (std::find(full_sim_regions.begin(), full_sim_regions.end(), region_id) != full_sim_regions.end()) {
            // Already assigned to full
            continue;
        }
        
        // Check if visible on screen
        bool is_visible = IsRegionVisible(region_id);
        
        if (is_visible) {
            // Visible but not in focus = half simulation
            half_sim_regions.push_back(region_id);
        } else {
            // Off-screen = formula simulation
            formula_sim_regions.push_back(region_id);
        }
    }
    
    // Update LOD system - set focus regions and update LOD assignments
    simulation_manager_->SetFocusRegions(full_sim_regions);
    
    // Get LOD system to set LODs directly
    auto* lod_system = simulation_manager_->GetLODSystem();
    if (!lod_system) {
        return;
    }
    
    // Set Full simulation for focus regions
    for (RegionID region_id : full_sim_regions) {
        lod_system->SetRegionLOD(region_id, SimulationLOD::Full);
    }
    
    // Set Half simulation for visible regions
    for (RegionID region_id : half_sim_regions) {
        lod_system->SetRegionLOD(region_id, SimulationLOD::Half);
    }
    
    // Set Formula simulation for off-screen regions
    for (RegionID region_id : formula_sim_regions) {
        lod_system->SetRegionLOD(region_id, SimulationLOD::Formula);
    }
}

std::vector<RegionID> WorldScene::GetNeighborRegions(RegionID region_id, u8 range) const {
    std::vector<RegionID> neighbors;
    
    if (!simulation_manager_ || region_id == INVALID_REGION_ID) {
        return neighbors;
    }
    
    // Get grid position of selected region
    u16 center_x, center_y;
    GetRegionGridPosition(region_id, center_x, center_y);
    
    // Get all regions within range (including diagonals)
    for (i16 dy = -static_cast<i16>(range); dy <= static_cast<i16>(range); ++dy) {
        for (i16 dx = -static_cast<i16>(range); dx <= static_cast<i16>(range); ++dx) {
            // Skip center (selected region itself)
            if (dx == 0 && dy == 0) {
                continue;
            }
            
            u16 grid_x = static_cast<u16>(static_cast<i16>(center_x) + dx);
            u16 grid_y = static_cast<u16>(static_cast<i16>(center_y) + dy);
            
            // Check bounds
            if (grid_x >= grid_width_ || grid_y >= grid_height_) {
                continue;
            }
            
            RegionID neighbor_id = GetRegionAtGridPosition(grid_x, grid_y);
            if (neighbor_id != INVALID_REGION_ID) {
                neighbors.push_back(neighbor_id);
            }
        }
    }
    
    return neighbors;
}

bool WorldScene::IsRegionVisible(RegionID region_id) const {
    if (!simulation_manager_ || region_id == INVALID_REGION_ID) {
        return false;
    }
    
    const Simulation::Region* region = simulation_manager_->GetRegion(region_id);
    if (!region) {
        return false;
    }
    
    // Get frame dimensions (viewport dimensions when rendered)
    i32 frame_x, frame_y, frame_width, frame_height;
    GetFrameBounds(frame_x, frame_y, frame_width, frame_height);
    
    // Calculate view bounds
    f32 view_left = camera_x_ - (frame_width / 2.0f) / zoom_level_;
    f32 view_right = camera_x_ + (frame_width / 2.0f) / zoom_level_;
    f32 view_top = camera_y_ - (frame_height / 2.0f) / zoom_level_;
    f32 view_bottom = camera_y_ + (frame_height / 2.0f) / zoom_level_;
    
    f32 world_x = region->GetX();
    f32 world_y = region->GetY();
    
    // Check if region is visible
    return !(world_x + region_size_ < view_left || world_x > view_right ||
             world_y + region_size_ < view_top || world_y > view_bottom);
}

void WorldScene::GetRegionGridPosition(RegionID region_id, u16& grid_x, u16& grid_y) const {
    if (!simulation_manager_ || region_id == INVALID_REGION_ID) {
        grid_x = grid_y = 0;
        return;
    }
    
    const Simulation::Region* region = simulation_manager_->GetRegion(region_id);
    if (!region) {
        grid_x = grid_y = 0;
        return;
    }
    
    // Convert world position to grid position
    f32 world_x = region->GetX();
    f32 world_y = region->GetY();
    
    grid_x = static_cast<u16>(world_x / region_size_);
    grid_y = static_cast<u16>(world_y / region_size_);
}

RegionID WorldScene::GetRegionAtGridPosition(u16 grid_x, u16 grid_y) const {
    if (!simulation_manager_ || grid_x >= grid_width_ || grid_y >= grid_height_) {
        return INVALID_REGION_ID;
    }
    
    // Calculate region index from grid position
    u32 region_index = static_cast<u32>(grid_y) * static_cast<u32>(grid_width_) + static_cast<u32>(grid_x);
    
    const auto& regions = simulation_manager_->GetRegions();
    if (region_index < regions.size()) {
        return regions[region_index]->GetID();
    }
    
    return INVALID_REGION_ID;
}


} // namespace Game
