#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace Simulation {

// Forward declarations
class Region;
class LODSystem;

// Simulation manager - orchestrates all simulation layers
class SimulationManager {
public:
    SimulationManager();
    ~SimulationManager();
    
    // Initialize simulation
    bool Initialize();
    
    // Update simulation (called every frame)
    void Update(f32 delta_time);
    
    // Get/set focus regions
    void SetFocusRegions(const std::vector<RegionID>& regions);
    std::vector<RegionID> GetFocusRegions() const;
    
    // Get region by ID
    Region* GetRegion(RegionID region_id);
    const Region* GetRegion(RegionID region_id) const;
    
    // Get all regions
    const std::vector<std::unique_ptr<Region>>& GetRegions() const;
    
    // Get current simulation tick
    Tick GetCurrentTick() const { return current_tick_; }
    
    // Pause/resume
    void Pause();
    void Resume();
    bool IsPaused() const { return is_paused_; }
    
    // Set time scale
    void SetTimeScale(f32 scale);
    f32 GetTimeScale() const { return time_scale_; }
    
    // Initialize region grid
    void InitializeRegionGrid(u16 grid_width, u16 grid_height, f32 region_size);
    
    // Update LOD system (called by WorldScene)
    void UpdateLOD();
    
    // Get LOD system (for WorldScene to set LODs directly)
    LODSystem* GetLODSystem() { return lod_system_.get(); }
    
private:
    void UpdateRegions(f32 delta_time);
    void ProcessLODTransitions();
    
    std::vector<std::unique_ptr<Region>> regions_;
    std::vector<RegionID> focus_regions_;
    std::unique_ptr<LODSystem> lod_system_;
    
    Tick current_tick_ = 0;
    f32 time_scale_ = 1.0f;
    bool is_paused_ = false;
    f32 accumulated_time_ = 0.0f;
};

} // namespace Simulation

