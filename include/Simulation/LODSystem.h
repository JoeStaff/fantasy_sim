#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include <vector>
#include <unordered_map>

namespace Simulation {

// LOD (Level of Detail) system
class LODSystem {
public:
    LODSystem();
    ~LODSystem();
    
    // Initialize LOD system
    void Initialize();
    
    // Update LOD assignments based on focus regions
    void UpdateLOD(const std::vector<RegionID>& focus_regions, u8 visible_region_count);
    
    // Get LOD level for a region
    SimulationLOD GetRegionLOD(RegionID region_id) const;
    
    // Check if region should update this tick
    bool ShouldUpdateRegion(RegionID region_id, Tick current_tick) const;
    
    // Get regions at a specific LOD level
    std::vector<RegionID> GetRegionsAtLOD(SimulationLOD lod) const;
    
    // Transition region to new LOD
    void TransitionRegion(RegionID region_id, SimulationLOD new_lod);
    
private:
    struct RegionLODData {
        SimulationLOD current_lod = SimulationLOD::Formula;
        Tick last_update_tick = 0;
        u32 update_counter = 0;
    };
    
    std::unordered_map<RegionID, RegionLODData> region_lod_data_;
    
    u32 GetUpdateFrequency(SimulationLOD lod) const;
};

} // namespace Simulation
