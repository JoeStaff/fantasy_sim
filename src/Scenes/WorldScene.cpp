#include "Scenes/WorldScene.h"
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
    
    // Set viewport for main world view (leave space for sidebar)
    i32 window_width = video->GetWindowWidth();
    i32 window_height = video->GetWindowHeight();
    video->SetViewport(0, 0, window_width - SIDEBAR_WIDTH, window_height);
    
    // Render regions
    RenderRegions(video);
    
    // Reset viewport and render sidebar
    video->ResetViewport();
    RenderSidebar(video);
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
    camera_x_ = (grid_width_ * region_size_) / 2.0f;
    camera_y_ = (grid_height_ * region_size_) / 2.0f;
    zoom_level_ = 1.0f;
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
    
    RegionID region_id = GetRegionAtScreenPosition(mouse_x, mouse_y);
    if (region_id != INVALID_REGION_ID) {
        // Single selection - if clicking same region, deselect; otherwise select new one
        if (selected_region_id_ == region_id) {
            selected_region_id_ = INVALID_REGION_ID;
        } else {
            selected_region_id_ = region_id;
        }
        
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
    
    i32 window_width = video->GetWindowWidth();
    i32 window_height = video->GetWindowHeight();
    
    f32 scaled_region_size = region_size_ * zoom_level_;
    
    // Calculate visible region bounds (in world coordinates)
    // Account for sidebar width in viewport
    i32 viewport_width = window_width - SIDEBAR_WIDTH;
    f32 view_left = camera_x_ - (viewport_width / 2.0f) / zoom_level_;
    f32 view_right = camera_x_ + (viewport_width / 2.0f) / zoom_level_;
    f32 view_top = camera_y_ - (window_height / 2.0f) / zoom_level_;
    f32 view_bottom = camera_y_ + (window_height / 2.0f) / zoom_level_;
    
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
        
        // Only render if region is at least partially on screen (with some margin)
        if (screen_x + screen_size < -10 || screen_x > window_width + 10 ||
            screen_y + screen_size < -10 || screen_y > window_height + 10) {
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
    if (!platform_manager_ || !platform_manager_->GetVideo()) {
        screen_x = screen_y = 0;
        return;
    }
    
    auto* video = platform_manager_->GetVideo();
    i32 window_width = video->GetWindowWidth();
    i32 window_height = video->GetWindowHeight();
    i32 viewport_width = window_width - SIDEBAR_WIDTH;
    
    // Convert world coordinates to screen coordinates
    // Account for sidebar - center viewport horizontally
    f32 screen_fx = (world_x - camera_x_) * zoom_level_ + (viewport_width / 2.0f);
    f32 screen_fy = (world_y - camera_y_) * zoom_level_ + (window_height / 2.0f);
    
    screen_x = static_cast<i32>(screen_fx);
    screen_y = static_cast<i32>(screen_fy);
}

void WorldScene::ScreenToWorld(i32 screen_x, i32 screen_y, f32& world_x, f32& world_y) {
    if (!platform_manager_ || !platform_manager_->GetVideo()) {
        world_x = world_y = 0.0f;
        return;
    }
    
    auto* video = platform_manager_->GetVideo();
    i32 window_width = video->GetWindowWidth();
    i32 window_height = video->GetWindowHeight();
    i32 viewport_width = window_width - SIDEBAR_WIDTH;
    
    // Convert screen coordinates to world coordinates
    // Account for sidebar - center viewport horizontally
    world_x = (screen_x - (viewport_width / 2.0f)) / zoom_level_ + camera_x_;
    world_y = (screen_y - (window_height / 2.0f)) / zoom_level_ + camera_y_;
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
    
    if (!platform_manager_ || !platform_manager_->GetVideo()) {
        return false;
    }
    
    auto* video = platform_manager_->GetVideo();
    i32 window_width = video->GetWindowWidth();
    i32 window_height = video->GetWindowHeight();
    
    // Calculate view bounds
    f32 view_left = camera_x_ - (window_width / 2.0f) / zoom_level_;
    f32 view_right = camera_x_ + (window_width / 2.0f) / zoom_level_;
    f32 view_top = camera_y_ - (window_height / 2.0f) / zoom_level_;
    f32 view_bottom = camera_y_ + (window_height / 2.0f) / zoom_level_;
    
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

void WorldScene::RenderSidebar(Platform::IVideo* video) {
    if (!video) {
        return;
    }
    
    i32 window_width = video->GetWindowWidth();
    i32 window_height = video->GetWindowHeight();
    i32 sidebar_x = window_width - SIDEBAR_WIDTH;
    
    // Draw sidebar background
    video->SetDrawColor(40, 40, 50, 255);
    video->DrawRect(sidebar_x, 0, SIDEBAR_WIDTH, window_height);
    
    // Draw sidebar border
    video->SetDrawColor(60, 60, 70, 255);
    video->DrawLine(sidebar_x, 0, sidebar_x, window_height);
    
    // Render region stats if a region is selected
    if (selected_region_id_ != INVALID_REGION_ID && simulation_manager_) {
        const Simulation::Region* region = simulation_manager_->GetRegion(selected_region_id_);
        if (region) {
            RenderRegionStats(video, region);
        }
    } else {
        // No region selected - show placeholder text
        video->SetDrawColor(200, 200, 200, 255);
        video->DrawText("No region selected", sidebar_x + 10, 20, 200, 200, 200, 255);
        video->DrawText("Click a region to view stats", sidebar_x + 10, 50, 150, 150, 150, 255);
    }
}

void WorldScene::RenderRegionStats(Platform::IVideo* video, const Simulation::Region* region) {
    if (!video || !region) {
        return;
    }
    
    i32 window_width = video->GetWindowWidth();
    i32 sidebar_x = window_width - SIDEBAR_WIDTH;
    i32 y_pos = 20;
    const i32 line_height = 25;
    const i32 title_size = 18;
    const i32 text_size = 14;
    
    // Title
    video->SetFontSize(title_size);
    video->SetDrawColor(255, 255, 255, 255);
    video->DrawText("Region Stats", sidebar_x + 10, y_pos, 255, 255, 255, 255);
    y_pos += line_height + 10;
    
    // Draw separator line
    video->SetDrawColor(100, 100, 100, 255);
    video->DrawLine(sidebar_x + 10, y_pos, sidebar_x + SIDEBAR_WIDTH - 10, y_pos);
    y_pos += 15;
    
    video->SetFontSize(text_size);
    
    // Region Name (if it's a source region)
    const std::string& name = region->GetName();
    if (!name.empty()) {
        video->SetDrawColor(255, 255, 200, 255);  // Highlight name
        video->DrawText("Name: " + name, sidebar_x + 10, y_pos, 255, 255, 200, 255);
        y_pos += line_height;
    }
    
    // Settlement Role (if it's a settlement)
    if (simulation_manager_) {
        const Simulation::World* world = simulation_manager_->GetWorld();
        if (world) {
            const auto& settlements = world->GetSettlements();
            for (const auto& settlement : settlements) {
                if (settlement.region_id == region->GetID()) {
                    std::string role = settlement.type;
                    // Determine role description based on context
                    if (settlement.type == "City") {
                        // Check neighbors to determine role
                        bool near_mountain = false;
                        bool near_water = false;
                        bool near_forest = false;
                        
                        u16 grid_x = settlement.grid_x;
                        u16 grid_y = settlement.grid_y;
                        
                        if (world) {
                            const Simulation::Region* neighbors[4] = {
                                world->GetRegionAtGrid(grid_x, grid_y - 1),
                                world->GetRegionAtGrid(grid_x, grid_y + 1),
                                world->GetRegionAtGrid(grid_x - 1, grid_y),
                                world->GetRegionAtGrid(grid_x + 1, grid_y)
                            };
                            
                            for (const Simulation::Region* neighbor : neighbors) {
                                if (neighbor) {
                                    if (neighbor->GetType() == "Mountain") near_mountain = true;
                                    if (neighbor->GetType() == "Coastal" || neighbor->GetType() == "River") near_water = true;
                                    if (neighbor->GetType() == "Forest") near_forest = true;
                                }
                            }
                        }
                        
                        if (near_mountain) role = "Mountain Settlement";
                        else if (near_water) role = "Coastal Settlement";
                        else if (near_forest) role = "Forest Settlement";
                        else role = "Plains Settlement";
                    } else if (settlement.type == "Village") {
                        role = "Village";
                    } else if (settlement.type == "Capital") {
                        role = "Capital";
                    }
                    
                    video->SetDrawColor(200, 255, 200, 255);  // Highlight settlement role
                    video->DrawText("Role: " + role, sidebar_x + 10, y_pos, 200, 255, 200, 255);
                    y_pos += line_height;
                    break;
                }
            }
        }
    }
    
    // Region ID
    video->SetDrawColor(200, 200, 200, 255);
    video->DrawText("ID: " + std::to_string(region->GetID()), sidebar_x + 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    
    // Region Type
    std::string type = region->GetType();
    video->DrawText("Type: " + type, sidebar_x + 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    
    // Region Subtype (if exists)
    const std::string& subtype = region->GetSubtype();
    if (!subtype.empty()) {
        video->DrawText("Subtype: " + subtype, sidebar_x + 10, y_pos, 200, 200, 200, 255);
        y_pos += line_height;
    }
    
    // Source region info
    if (region->IsSource()) {
        video->SetDrawColor(180, 200, 255, 255);
        video->DrawText("Source Region", sidebar_x + 10, y_pos, 180, 200, 255, 255);
        y_pos += line_height;
    } else if (region->GetSourceParentID() != INVALID_REGION_ID) {
        const Simulation::Region* parent = simulation_manager_ ? 
            simulation_manager_->GetRegion(region->GetSourceParentID()) : nullptr;
        if (parent) {
            video->SetDrawColor(180, 200, 255, 255);
            std::string parent_name = parent->GetName();
            if (parent_name.empty()) {
                parent_name = parent->GetType();
            }
            video->DrawText("Part of: " + parent_name, sidebar_x + 10, y_pos, 180, 200, 255, 255);
            y_pos += line_height;
        }
    }
    
    y_pos += 5;  // Small gap
    
    // Population
    u32 population = region->GetPopulation();
    u32 capacity = region->GetCapacity();
    video->SetDrawColor(200, 200, 200, 255);
    video->DrawText("Population: " + std::to_string(population), sidebar_x + 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    
    // Capacity
    video->DrawText("Capacity: " + std::to_string(capacity), sidebar_x + 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    
    // Population percentage
    f32 pop_percent = capacity > 0 ? (static_cast<f32>(population) / static_cast<f32>(capacity)) * 100.0f : 0.0f;
    std::string pop_str = "Fullness: " + std::to_string(static_cast<int>(pop_percent)) + "%";
    video->DrawText(pop_str, sidebar_x + 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height + 10;
    
    // Position
    video->DrawText("Position:", sidebar_x + 10, y_pos, 200, 200, 200, 255);
    y_pos += line_height;
    std::string pos_str = "  X: " + std::to_string(static_cast<int>(region->GetX()));
    video->DrawText(pos_str, sidebar_x + 10, y_pos, 180, 180, 180, 255);
    y_pos += line_height;
    pos_str = "  Y: " + std::to_string(static_cast<int>(region->GetY()));
    video->DrawText(pos_str, sidebar_x + 10, y_pos, 180, 180, 180, 255);
    y_pos += line_height + 10;
    
    // Neighbors
    const auto& neighbors = region->GetNeighbors();
    video->DrawText("Neighbors: " + std::to_string(neighbors.size()), sidebar_x + 10, y_pos, 200, 200, 200, 255);
}

} // namespace Game
