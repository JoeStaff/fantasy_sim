#include "Simulation/LODSystem.h"

namespace Simulation {

LODSystem::LODSystem() = default;

LODSystem::~LODSystem() = default;

void LODSystem::Initialize() {
    // TODO: Implement initialization
}

void LODSystem::UpdateLOD(const std::vector<RegionID>& focus_regions, u8 visible_region_count) {
    // TODO: Implement LOD update
    (void)focus_regions;
    (void)visible_region_count;
}

SimulationLOD LODSystem::GetRegionLOD(RegionID region_id) const {
    auto it = region_lod_data_.find(region_id);
    if (it != region_lod_data_.end()) {
        return it->second.current_lod;
    }
    return SimulationLOD::Formula;
}

bool LODSystem::ShouldUpdateRegion(RegionID region_id, Tick current_tick) const {
    auto it = region_lod_data_.find(region_id);
    if (it != region_lod_data_.end()) {
        const auto& data = it->second;
        u32 frequency = GetUpdateFrequency(data.current_lod);
        return (current_tick - data.last_update_tick) >= frequency;
    }
    return false;
}

std::vector<RegionID> LODSystem::GetRegionsAtLOD(SimulationLOD lod) const {
    std::vector<RegionID> result;
    for (const auto& pair : region_lod_data_) {
        if (pair.second.current_lod == lod) {
            result.push_back(pair.first);
        }
    }
    return result;
}

void LODSystem::TransitionRegion(RegionID region_id, SimulationLOD new_lod) {
    auto& data = region_lod_data_[region_id];
    data.current_lod = new_lod;
    // TODO: Implement smooth transitions
}

u32 LODSystem::GetUpdateFrequency(SimulationLOD lod) const {
    // TODO: Get from config
    switch (lod) {
        case SimulationLOD::Full:
            return 1;
        case SimulationLOD::Half:
            return 3;
        case SimulationLOD::Formula:
            return 30;
        default:
            return 30;
    }
}

} // namespace Simulation
