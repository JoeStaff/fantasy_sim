#pragma once

#include "Simulation/WorldGenerator.h"
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
    // Generation passes
    void Pass1_InitializePlains(World* world);
    void Pass2_DetermineCoastalBorders(World* world);
    void Pass3_DetermineMountains(World* world);
    void Pass4_DetermineForests(World* world);
    void Pass5_DetermineRivers(World* world);
    void Pass6_DetermineDesert(World* world);
    void Pass7_SprinkleFreshWater(World* world);
    void Pass8_PlaceSettlements(World* world);
    void Pass9_PathRoads(World* world);
    
    // Source-based generation methods
    void GenerateSourceRegions(World* world, const std::string& region_type, const RegionDefinition& def);
    void ExpandFromSource(World* world, RegionID source_id, const RegionDefinition& def);
    void EnsureCompleteCoastalBorders(World* world);
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
    
    // Track which borders have coastal regions (top, bottom, left, right)
    std::unordered_set<std::string> coastal_borders_;

}; // class StandardWorldGenerator

} // namespace Simulation

