#pragma once

#include "Core/Types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Simulation {

// Region definition loaded from JSON
struct RegionDefinition {
    std::string type;  // Region type identifier (e.g., "Forest", "Mountain")
    
    // Generation weights
    f32 spawn_weight = 1.0f;  // Base weight for spawning this region type
    f32 expansion_weight = 1.0f;  // Weight for expanding from source
    
    // Visual
    u8 color_r = 128;
    u8 color_g = 128;
    u8 color_b = 128;
    
    // Names
    std::vector<std::string> potential_names;  // List of potential names for source regions
    
    // Generation parameters
    u32 min_source_count = 1;  // Minimum number of source regions
    u32 max_source_count = 5;  // Maximum number of source regions
    u32 min_expansion_size = 5;  // Minimum cells per source
    u32 max_expansion_size = 30;  // Maximum cells per source
    
    // Influence stats (for source regions)
    std::unordered_map<std::string, f32> influence_stats;  // Stats that influence surrounding areas
    
    // Other properties
    u32 capacity = 10000;  // Default capacity
    std::vector<std::string> resource_types;  // Available resources
    std::vector<std::string> compatible_neighbors;  // Types that can be neighbors
    std::vector<std::string> incompatible_neighbors;  // Types that cannot be neighbors
};

} // namespace Simulation


