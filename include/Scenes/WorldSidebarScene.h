#pragma once

#include "Scenes/Scene.h"
#include "Core/Types.h"
#include "Core/Config.h"
#include "Simulation/SimulationManager.h"
#include <memory>

namespace Game {

// Simple shared state for selected region (shared between WorldScene and WorldSidebarScene)
namespace WorldSceneSharedState {
    extern RegionID g_selected_region_id;
    extern Simulation::SimulationManager* g_simulation_manager;
}

// Sidebar scene - displays region statistics
// This was previously part of WorldScene
class WorldSidebarScene : public Scene {
public:
    WorldSidebarScene();
    virtual ~WorldSidebarScene() = default;
    
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
    // Rendering
    void RenderRegionStats(Platform::IVideo* video, const Simulation::Region* region);
    
    // Sidebar dimensions
    static constexpr i32 SIDEBAR_WIDTH = 300;
};

} // namespace Game

