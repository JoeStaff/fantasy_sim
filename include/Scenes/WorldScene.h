#pragma once

#include "Scenes/Scene.h"
#include "Core/Types.h"
#include "Core/Config.h"
#include "Simulation/SimulationManager.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace Game {

// World scene - displays the world map with regions
// Allows scrolling, zooming, and region selection
class WorldScene : public Scene {
public:
    WorldScene();
    virtual ~WorldScene() = default;
    
    // Scene interface
    bool Initialize(Platform::PlatformManager* platform_manager) override;
    void Shutdown() override;
    void Update(f32 delta_time) override;
    void Render(Platform::IVideo* video) override;
    void ProcessInput(Platform::IInput* input) override;
    
    // Scene transition callbacks
    void OnEnter() override;
    void OnExit() override;
    
private:
    // Camera/viewport
    f32 camera_x_ = 0.0f;
    f32 camera_y_ = 0.0f;
    f32 zoom_level_ = 1.0f;
    static constexpr f32 MIN_ZOOM = 0.1f;
    static constexpr f32 MAX_ZOOM = 5.0f;
    static constexpr f32 ZOOM_SPEED = 0.1f;
    static constexpr f32 SCROLL_SPEED = 500.0f;  // pixels per second
    
    // Region grid
    u16 grid_width_ = 100;
    u16 grid_height_ = 100;
    f32 region_size_ = 50.0f;  // Base size in pixels (will be scaled by zoom)
    
    // Selected region (single selection only)
    RegionID selected_region_id_ = INVALID_REGION_ID;
    
    // Simulation manager
    std::unique_ptr<Simulation::SimulationManager> simulation_manager_;
    
    // Region type colors (for rendering)
    std::unordered_map<std::string, std::tuple<u8, u8, u8>> region_colors_;
    
    // Input handling
    void HandleScrolling(f32 delta_time, Platform::IInput* input);
    void HandleZooming(f32 delta_time, Platform::IInput* input);
    void HandleRegionSelection(Platform::IInput* input);
    
    // Simulation LOD management
    void UpdateSimulationLOD();
    std::vector<RegionID> GetNeighborRegions(RegionID region_id, u8 range) const;
    bool IsRegionVisible(RegionID region_id) const;
    
    // Rendering
    void RenderRegions(Platform::IVideo* video);
    void RenderRegion(Platform::IVideo* video, Simulation::Region* region, 
                      i32 screen_x, i32 screen_y, i32 screen_size);
    void GetRegionColor(const std::string& region_type, u8& r, u8& g, u8& b);
    
    // Coordinate conversion
    void WorldToScreen(f32 world_x, f32 world_y, i32& screen_x, i32& screen_y);
    void ScreenToWorld(i32 screen_x, i32 screen_y, f32& world_x, f32& world_y);
    RegionID GetRegionAtScreenPosition(i32 screen_x, i32 screen_y);
    
    // Grid helpers
    void GetRegionGridPosition(RegionID region_id, u16& grid_x, u16& grid_y) const;
    RegionID GetRegionAtGridPosition(u16 grid_x, u16 grid_y) const;
    
    // Initialization
    void InitializeRegionColors();
    void InitializeRegions();
    
    // Helper to get region type from region
    std::string GetRegionType(Simulation::Region* region);
};

} // namespace Game
