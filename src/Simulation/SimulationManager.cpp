#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include <utility>
#include <limits>

#include "Simulation/SimulationManager.h"
#include "Simulation/LODSystem.h"
#include "Simulation/Region.h"
#include "Simulation/World.h"
#include "Simulation/WorldGenerator.h"
#include "Simulation/StandardWorldGenerator.h"
#include "Core/Config.h"
#include "Utils/Random.h"

namespace Simulation {

SimulationManager::SimulationManager() = default;

SimulationManager::~SimulationManager() = default;

bool SimulationManager::Initialize() {
    // Initialize LOD system
    lod_system_ = std::make_unique<LODSystem>();
    if (lod_system_) {
        lod_system_->Initialize();
    }
    
    return true;
}

void SimulationManager::Update(f32 delta_time) {
    // TODO: Implement update logic
    (void)delta_time;
}

void SimulationManager::SetFocusRegions(const std::vector<RegionID>& regions) {
    focus_regions_ = regions;
}

std::vector<RegionID> SimulationManager::GetFocusRegions() const {
    return focus_regions_;
}

Region* SimulationManager::GetRegion(RegionID region_id) {
    if (!world_) {
        return nullptr;
    }
    return world_->GetRegion(region_id);
}

const Region* SimulationManager::GetRegion(RegionID region_id) const {
    if (!world_) {
        return nullptr;
    }
    return world_->GetRegion(region_id);
}

const std::vector<std::unique_ptr<Region>>& SimulationManager::GetRegions() const {
    if (!world_) {
        static std::vector<std::unique_ptr<Region>> empty_regions;
        return empty_regions;
    }
    return world_->GetRegions();
}

void SimulationManager::Pause() {
    is_paused_ = true;
}

void SimulationManager::Resume() {
    is_paused_ = false;
}

void SimulationManager::SetTimeScale(f32 scale) {
    time_scale_ = scale;
}

void SimulationManager::UpdateLOD() {
    if (!lod_system_) {
        return;
    }
    
    auto& config = Config::Configuration::GetInstance();
    u8 visible_region_count = config.simulation.lod.visible_region_count;
    
    // Update LOD system with current focus regions
    lod_system_->UpdateLOD(focus_regions_, visible_region_count);
}

void SimulationManager::UpdateRegions(f32 delta_time) {
    // TODO: Implement region updates
    (void)delta_time;
}

void SimulationManager::ProcessLODTransitions() {
    // TODO: Implement LOD transitions
}

void SimulationManager::InitializeRegionGrid(u16 grid_width, u16 grid_height, f32 region_size) {
    std::cout << "SimulationManager: Initializing region grid using world generator..." << std::endl;
    
    // Create world generator
    world_generator_ = std::make_unique<StandardWorldGenerator>();
    
    // Generate world
    world_ = world_generator_->Generate(grid_width, grid_height, region_size);
    
    if (!world_) {
        std::cout << "SimulationManager: ERROR - Failed to generate world!" << std::endl;
        return;
    }
    
    std::cout << "SimulationManager: World generated successfully" << std::endl;
    std::cout << "SimulationManager: Created " << world_->GetRegions().size() << " regions" << std::endl;
}

} // namespace Simulation

