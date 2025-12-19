#pragma once

#include "Simulation/WorldGenerator.h"
#include "Simulation/RegionDefinition.h"
#include "Utils/Random.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace Simulation {

// Standard world generator - creates a world with multiple passes
class StandardWorldGenerator : public WorldGenerator {
public:
    StandardWorldGenerator();
    virtual ~StandardWorldGenerator() = default;
    
    // Generate a world
    std::unique_ptr<World> Generate(u16 grid_width, u16 grid_height, f32 region_size) override;
    
private:
    // Pass 0: Initialize base layer
    void Pass0_InitializePlains(World* world);
    
    // Determine generation order from region definitions
    std::vector<std::string> DetermineGenerationOrder(
        const std::unordered_map<std::string, RegionDefinition>& region_definitions);
    
    // Core pass methods: create sources, then expand
    std::vector<RegionID> Pass_CreateSources(World* world, const std::string& region_type, const RegionDefinition& def);
    void Pass_ExpandFromSource(World* world, RegionID source_id, const RegionDefinition& def);
    
    // Expansion helpers
    void ExpandCoastalInland(World* world, RegionID source_id, const RegionDefinition& def, u16 source_x, u16 source_y);
    void ExpandStandardRegion(World* world, RegionID source_id, const RegionDefinition& def, u16 source_x, u16 source_y);
    
    // Special passes for regions that don't follow standard pattern
    void Pass_Coastal(World* world, const RegionDefinition& def);
    void Pass_Rivers(World* world, const std::unordered_map<std::string, RegionDefinition>& region_definitions);
    std::vector<RegionID> Pass_CreateRiverSources(World* world, const RegionDefinition& def);
    void Pass_ExpandRiverFromSource(World* world, RegionID source_id, const RegionDefinition& def);
    
    void Pass_Settlements(World* world, const std::unordered_map<std::string, RegionDefinition>& region_definitions);
    void Pass_Roads(World* world, const std::unordered_map<std::string, RegionDefinition>& region_definitions);
    
    // Utility methods
    std::string GetRandomName(const RegionDefinition& def);
    
    // Helper methods
    Region* GetRegionAtGrid(World* world, u16 gx, u16 gy);
    bool IsOnRim(u16 x, u16 y) const;
    bool IsInNorthernHemisphere(u16 y) const;
    std::vector<std::pair<u16, u16>> FindPath(World* world, 
                                                const std::pair<u16, u16>& start,
                                                const std::pair<u16, u16>& end);
    std::vector<std::pair<u16, u16>> FindPathWithMerging(World* world,
                                                          const std::pair<u16, u16>& start,
                                                          const std::pair<u16, u16>& end);
    std::vector<std::pair<u16, u16>> FindRoadPath(World* world,
                                                   const std::pair<u16, u16>& start,
                                                   const std::pair<u16, u16>& end);
    std::pair<u16, u16> FindNearestWaterSource(World* world, u16 x, u16 y);
    std::pair<u16, u16> FindNearestNonCoastalBorder(World* world);
    std::pair<u16, u16> CalculateCentroid(const std::vector<std::pair<u16, u16>>& positions);
    bool CanPlaceRegion(World* world, u16 x, u16 y, const RegionDefinition& def);
    
    // Grid dimensions
    u16 grid_width_ = 0;
    u16 grid_height_ = 0;
    f32 region_size_ = 0.0f;
    
    // Random instance
    Utils::Random* random_ = nullptr;
    
    // Desert hemisphere tracking (true = northern, false = southern)
    bool desert_northern_hemisphere_ = false;
    bool desert_hemisphere_set_ = false;

    // Forest hemisphere tracking (true = northern, false = southern)
    bool forest_northern_hemisphere_ = false;
    bool forest_hemisphere_set_ = false;
    
    // Track which borders have coastal regions (top, bottom, left, right)
    std::unordered_set<std::string> coastal_borders_;

}; // class StandardWorldGenerator

} // namespace Simulation


