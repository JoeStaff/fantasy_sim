#pragma once

#include "Core/Types.h"
#include "Simulation/Region.h"
#include <vector>
#include <memory>
#include <string>

namespace Simulation {

// World class - holds the generated world data
class World {
public:
    World();
    ~World();
    
    // Initialize world with grid dimensions
    void Initialize(u16 grid_width, u16 grid_height, f32 region_size);
    
    // Get grid dimensions
    u16 GetGridWidth() const { return grid_width_; }
    u16 GetGridHeight() const { return grid_height_; }
    f32 GetRegionSize() const { return region_size_; }
    
    // Region access
    Region* GetRegion(RegionID region_id);
    const Region* GetRegion(RegionID region_id) const;
    Region* GetRegionAtGrid(u16 grid_x, u16 grid_y);
    const Region* GetRegionAtGrid(u16 grid_x, u16 grid_y) const;
    
    // Get all regions
    const std::vector<std::unique_ptr<Region>>& GetRegions() const { return regions_; }
    std::vector<std::unique_ptr<Region>>& GetRegions() { return regions_; }
    
    // Settlements (cities, villages, capital)
    struct Settlement {
        RegionID region_id;
        std::string type;  // "City", "Village", "Capital"
        u16 grid_x;
        u16 grid_y;
    };
    
    void AddSettlement(const Settlement& settlement);
    const std::vector<Settlement>& GetSettlements() const { return settlements_; }
    
    // Roads (connections between settlements)
    struct Road {
        RegionID from_region;
        RegionID to_region;
        std::vector<std::pair<u16, u16>> path;  // Grid coordinates along the path
    };
    
    void AddRoad(const Road& road);
    const std::vector<Road>& GetRoads() const { return roads_; }
    
    // Source regions (main parent regions for structures)
    void AddSourceRegion(RegionID region_id);
    const std::vector<RegionID>& GetSourceRegions() const { return source_regions_; }
    std::vector<RegionID> GetRegionsForSource(RegionID source_id) const;  // Get all regions belonging to a source
    
private:
    u16 grid_width_ = 0;
    u16 grid_height_ = 0;
    f32 region_size_ = 0.0f;
    
    std::vector<std::unique_ptr<Region>> regions_;
    std::vector<Settlement> settlements_;
    std::vector<Road> roads_;
    std::vector<RegionID> source_regions_;  // List of source region IDs
};

} // namespace Simulation



