#include "Simulation/StandardWorldGenerator.h"
#include "Simulation/World.h"
#include "Simulation/Region.h"
#include "Simulation/RegionDefinition.h"
#include "Simulation/RegionDefinitionLoader.h"
#include "Core/Config.h"
#include "Utils/Random.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <unordered_map>
#include <queue>

namespace Simulation {

StandardWorldGenerator::StandardWorldGenerator() {
    random_ = &Utils::Random::GetInstance();
}

std::unique_ptr<World> StandardWorldGenerator::Generate(u16 grid_width, u16 grid_height, f32 region_size) {
    grid_width_ = grid_width;
    grid_height_ = grid_height;
    region_size_ = region_size;
    
    // Reset desert hemisphere tracking for new generation
    desert_hemisphere_set_ = false;
    desert_northern_hemisphere_ = false;
    
    // Clear coastal borders tracking for new generation
    coastal_borders_.clear();
    
    auto world = std::make_unique<World>();
    world->Initialize(grid_width, grid_height, region_size);
    
    std::cout << "StandardWorldGenerator: Starting world generation" << std::endl;
    
    // Get region definitions from config
    auto& config = Config::Configuration::GetInstance();
    
    // Load region definitions if not already loaded
    if (config.regions.region_definitions.empty()) {
        LoadRegionDefinitions(config.regions);
    }
    
    const auto& region_definitions = config.regions.region_definitions;
    
    if (region_definitions.empty()) {
        std::cout << "StandardWorldGenerator: WARNING - No region definitions loaded! Using defaults." << std::endl;
        // Fall back to old method if no definitions
        Pass1_InitializePlains(world.get());
        Pass2_DetermineCoastalBorders(world.get());
        Pass3_DetermineMountains(world.get());
        Pass4_DetermineForests(world.get());
        Pass5_DetermineRivers(world.get());
        Pass6_DetermineDesert(world.get());
        Pass7_SprinkleFreshWater(world.get());
        Pass8_PlaceSettlements(world.get());
        Pass9_PathRoads(world.get());
    } else {
        // New source-based generation
        Pass1_InitializePlains(world.get());
        
        // Generate source regions for each region type that has source generation enabled
        for (const auto& [type, def] : region_definitions) {
            if (def.max_source_count > 0 && def.spawn_weight > 0.0f) {
                GenerateSourceRegions(world.get(), type, def);
            }
        }
        
        // Expand from all source regions
        const auto& source_regions = world->GetSourceRegions();
        std::cout << "Expanding from " << source_regions.size() << " source regions..." << std::endl;
        u32 expanded_count = 0;
        for (RegionID source_id : source_regions) {
            Region* source_region = world->GetRegion(source_id);
            if (source_region) {
                auto it = region_definitions.find(source_region->GetType());
                if (it != region_definitions.end()) {
                    ExpandFromSource(world.get(), source_id, it->second);
                    expanded_count++;
                    if (expanded_count % 10 == 0) {
                        std::cout << "Expanded " << expanded_count << "/" << source_regions.size() << " sources..." << std::endl;
                    }
                }
            }
        }
        std::cout << "Finished expanding from all source regions" << std::endl;
        
        // Ensure complete coastal border coverage
        EnsureCompleteCoastalBorders(world.get());
        
        // Special handling for rivers (they expand from RiverSource)
        Pass5_DetermineRivers(world.get());
        
        // Place settlements and roads
        Pass8_PlaceSettlements(world.get());
        
        // Expand Urban and Rural sources that were just created
        const auto& all_source_regions = world->GetSourceRegions();
        std::cout << "Expanding Urban and Rural sources..." << std::endl;
        for (RegionID source_id : all_source_regions) {
            Region* source_region = world->GetRegion(source_id);
            if (source_region && (source_region->GetType() == "Urban" || source_region->GetType() == "Rural")) {
                auto it = region_definitions.find(source_region->GetType());
                if (it != region_definitions.end()) {
                    ExpandFromSource(world.get(), source_id, it->second);
                }
            }
        }
        std::cout << "Finished expanding Urban and Rural sources" << std::endl;
        
        Pass9_PathRoads(world.get());
    }
    
    std::cout << "StandardWorldGenerator: World generation complete" << std::endl;
    std::cout << "StandardWorldGenerator: Created " << world->GetRegions().size() << " regions" << std::endl;
    std::cout << "StandardWorldGenerator: Created " << world->GetSourceRegions().size() << " source regions" << std::endl;
    std::cout << "StandardWorldGenerator: Created " << world->GetSettlements().size() << " settlements" << std::endl;
    std::cout << "StandardWorldGenerator: Created " << world->GetRoads().size() << " roads" << std::endl;
    
    return world;
}

void StandardWorldGenerator::GenerateSourceRegions(World* world, const std::string& region_type, const RegionDefinition& def) {
    std::cout << "Generating source regions for type: " << region_type << std::endl;
    
    // Special handling for Desert: randomly select a hemisphere (once per generation)
    if (region_type == "Desert" && !desert_hemisphere_set_) {
        desert_northern_hemisphere_ = random_->RandomBool(0.5f);
        desert_hemisphere_set_ = true;
        std::cout << "Desert will be placed in " << (desert_northern_hemisphere_ ? "Northern" : "Southern") << " hemisphere" << std::endl;
    }
    
    u32 source_count = random_->RandomU32(def.min_source_count, def.max_source_count + 1);
    u32 placed = 0;
    u32 attempts = 0;
    const u32 max_attempts = source_count * 100;
    
    while (placed < source_count && attempts < max_attempts) {
        attempts++;
        
        // Find a suitable location
        u16 x = static_cast<u16>(random_->RandomU32(0, grid_width_));
        u16 y = static_cast<u16>(random_->RandomU32(0, grid_height_));
        
        // Special handling for coastal - must be on rim
        if (region_type == "Coastal") {
            if (!IsOnRim(x, y)) {
                continue;
            }
        }
        
        // Special handling for Desert - must be in selected hemisphere
        if (region_type == "Desert" && desert_hemisphere_set_) {
            bool in_northern = IsInNorthernHemisphere(y);
            if (in_northern != desert_northern_hemisphere_) {
                continue;
            }
        }
        
        // Check if we can place here
        if (!CanPlaceRegion(world, x, y, def)) {
            continue;
        }
        
        Region* region = GetRegionAtGrid(world, x, y);
        if (!region) {
            continue;
        }
        
        // Create source region
        RegionID id = region->GetID();
        f32 wx = region->GetX();
        f32 wy = region->GetY();
        auto& regions = world->GetRegions();
        
        regions[id] = std::make_unique<Region>(id, region_type);
        regions[id]->SetPosition(wx, wy);
        regions[id]->SetIsSource(true);
        regions[id]->SetName(GetRandomName(def));
        regions[id]->Initialize();
        
        // Set capacity from definition
        // Note: Region doesn't have SetCapacity, so we'd need to add that or set it in Initialize
        
        world->AddSourceRegion(id);
        placed++;
    }
    
    std::cout << "Placed " << placed << " source regions for type: " << region_type << std::endl;
}

void StandardWorldGenerator::ExpandFromSource(World* world, RegionID source_id, const RegionDefinition& def) {
    Region* source_region = world->GetRegion(source_id);
    if (!source_region || !source_region->IsSource()) {
        return;
    }
    
    u16 source_x, source_y;
    {
        const Region* const_source = source_region;
        f32 world_x = const_source->GetX();
        f32 world_y = const_source->GetY();
        source_x = static_cast<u16>(world_x / region_size_);
        source_y = static_cast<u16>(world_y / region_size_);
    }
    
    // Special handling for coastal regions: cover entire border edge, then expand inland
    if (def.type == "Coastal" && IsOnRim(source_x, source_y)) {
        // Save source region name before we start replacing regions
        std::string source_name = source_region->GetName();
        
        // Determine which border edge(s) this source is on
        bool on_top = (source_y == 0);
        bool on_bottom = (source_y == grid_height_ - 1);
        bool on_left = (source_x == 0);
        bool on_right = (source_x == grid_width_ - 1);
        
        // Track which borders have coastal regions
        if (on_top) coastal_borders_.insert("top");
        if (on_bottom) coastal_borders_.insert("bottom");
        if (on_left) coastal_borders_.insert("left");
        if (on_right) coastal_borders_.insert("right");
        
        // Use a set to track unique border cells (handles corners where two edges meet)
        std::unordered_set<u32> border_cell_set;
        std::vector<std::pair<u16, u16>> border_cells_list;  // Also keep as list for expansion
        u32 border_cell_count = 0;
        
        if (on_top) {
            for (u16 x = 0; x < grid_width_; ++x) {
                u32 pos_key = static_cast<u32>(0) * static_cast<u32>(grid_width_) + static_cast<u32>(x);
                if (border_cell_set.insert(pos_key).second) {
                    border_cells_list.push_back({x, 0});
                    border_cell_count++;
                }
            }
        }
        if (on_bottom) {
            for (u16 x = 0; x < grid_width_; ++x) {
                u32 pos_key = static_cast<u32>(grid_height_ - 1) * static_cast<u32>(grid_width_) + static_cast<u32>(x);
                if (border_cell_set.insert(pos_key).second) {
                    border_cells_list.push_back({x, static_cast<u16>(grid_height_ - 1)});
                    border_cell_count++;
                }
            }
        }
        if (on_left) {
            for (u16 y = 0; y < grid_height_; ++y) {
                u32 pos_key = static_cast<u32>(y) * static_cast<u32>(grid_width_) + static_cast<u32>(0);
                if (border_cell_set.insert(pos_key).second) {
                    border_cells_list.push_back({0, y});
                    border_cell_count++;
                }
            }
        }
        if (on_right) {
            for (u16 y = 0; y < grid_height_; ++y) {
                u32 pos_key = static_cast<u32>(y) * static_cast<u32>(grid_width_) + static_cast<u32>(grid_width_ - 1);
                if (border_cell_set.insert(pos_key).second) {
                    border_cells_list.push_back({static_cast<u16>(grid_width_ - 1), y});
                    border_cell_count++;
                }
            }
        }
        
        // Place coastal regions on all border cells
        std::unordered_set<u32> placed_cells_set;
        std::vector<std::pair<u16, u16>> placed_cells;
        
        for (const auto& border_cell : border_cells_list) {
            u16 cell_x = border_cell.first;
            u16 cell_y = border_cell.second;
            Region* region = GetRegionAtGrid(world, cell_x, cell_y);
            if (region && (region->GetType() == "Plains" || region->GetType() == "Coastal")) {
                // Check compatibility
                if (CanPlaceRegion(world, cell_x, cell_y, def)) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, def.type);
                    regions[id]->SetPosition(wx, wy);
                    if (id == source_id) {
                        regions[id]->SetIsSource(true);
                        regions[id]->SetName(source_name);
                    } else {
                        regions[id]->SetSourceParentID(source_id);
                    }
                    regions[id]->Initialize();
                    
                    u32 pos_key = static_cast<u32>(cell_y) * static_cast<u32>(grid_width_) + static_cast<u32>(cell_x);
                    placed_cells_set.insert(pos_key);
                    placed_cells.push_back({cell_x, cell_y});
                }
            }
        }
        
        std::cout << "Expanded coastal source to cover " << border_cell_count << " border cells" << std::endl;
        
        // Now expand inland with distance-based strength
        // Calculate maximum inland distance (based on expansion size)
        u32 max_inland_distance = std::max(3u, def.max_expansion_size / 4u);  // At least 3 cells, up to 1/4 of max expansion
        
        // Base expansion probability for inland expansion
        f32 base_expand_prob = 0.3f;  // 30% base chance
        
        // Continue expansion from border cells inward
        u32 inland_placed = 0;
        const u32 max_inland_iterations = max_inland_distance * 50;
        u32 inland_iterations = 0;
        
        while (!placed_cells.empty() && inland_placed < def.max_expansion_size && inland_iterations < max_inland_iterations) {
            inland_iterations++;
            
            // Pick a random placed cell to expand from
            u32 seed_idx = random_->RandomU32(0, static_cast<u32>(placed_cells.size()));
            std::pair<u16, u16> current = placed_cells[seed_idx];
            
            // Calculate distance from nearest border
            u16 min_dist_to_border = std::min({
                current.second,  // distance to top
                static_cast<u16>(grid_height_ - 1 - current.second),  // distance to bottom
                current.first,  // distance to left
                static_cast<u16>(grid_width_ - 1 - current.first)  // distance to right
            });
            
            // Skip if too far inland
            if (min_dist_to_border >= max_inland_distance) {
                continue;
            }
            
            // Try to expand to neighbors (prefer moving away from border)
            std::vector<std::pair<u16, u16>> candidates;
            
            for (i16 dy = -1; dy <= 1; ++dy) {
                for (i16 dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    
                    i16 nx = static_cast<i16>(current.first) + dx;
                    i16 ny = static_cast<i16>(current.second) + dy;
                    
                    if (nx < 0 || nx >= static_cast<i16>(grid_width_) ||
                        ny < 0 || ny >= static_cast<i16>(grid_height_)) {
                        continue;
                    }
                    
                    u16 gx = static_cast<u16>(nx);
                    u16 gy = static_cast<u16>(ny);
                    u32 pos_key = static_cast<u32>(gy) * static_cast<u32>(grid_width_) + static_cast<u32>(gx);
                    
                    if (placed_cells_set.find(pos_key) != placed_cells_set.end()) {
                        continue;  // Already placed
                    }
                    
                    // Don't expand to other borders
                    if (IsOnRim(gx, gy)) {
                        continue;
                    }
                    
                    Region* region = GetRegionAtGrid(world, gx, gy);
                    if (!region) {
                        continue;
                    }
                    
                    // Check if we can expand here
                    if (region->GetType() != "Plains" && region->GetType() != def.type) {
                        continue;
                    }
                    
                    // Check compatibility
                    if (!CanPlaceRegion(world, gx, gy, def)) {
                        continue;
                    }
                    
                    candidates.push_back({gx, gy});
                }
            }
            
            // Try to place from candidates with distance-based probability
            for (const auto& candidate : candidates) {
                if (inland_placed >= def.max_expansion_size) {
                    break;
                }
                
                u32 pos_key = static_cast<u32>(candidate.second) * static_cast<u32>(grid_width_) + static_cast<u32>(candidate.first);
                if (placed_cells_set.find(pos_key) != placed_cells_set.end()) {
                    continue;
                }
                
                // Calculate distance from border for this candidate
                u16 candidate_dist = std::min({
                    candidate.second,
                    static_cast<u16>(grid_height_ - 1 - candidate.second),
                    candidate.first,
                    static_cast<u16>(grid_width_ - 1 - candidate.first)
                });
                
                // Probability decreases with distance from border
                // At border (dist=0): 100% chance, at max_inland_distance: ~10% chance
                f32 distance_factor = 1.0f - (static_cast<f32>(candidate_dist) / static_cast<f32>(max_inland_distance));
                distance_factor = std::max(0.1f, distance_factor);  // Minimum 10% chance
                
                f32 expand_prob = base_expand_prob * def.expansion_weight * distance_factor;
                expand_prob = std::min(1.0f, expand_prob);
                
                if (random_->RandomFloat(0.0f, 1.0f) < expand_prob) {
                    Region* region = GetRegionAtGrid(world, candidate.first, candidate.second);
                    if (region) {
                        RegionID id = region->GetID();
                        f32 wx = region->GetX();
                        f32 wy = region->GetY();
                        auto& regions = world->GetRegions();
                        
                        regions[id] = std::make_unique<Region>(id, def.type);
                        regions[id]->SetPosition(wx, wy);
                        regions[id]->SetSourceParentID(source_id);
                        regions[id]->Initialize();
                        
                        placed_cells_set.insert(pos_key);
                        placed_cells.push_back(candidate);
                        inland_placed++;
                    }
                }
            }
        }
        
        std::cout << "Coastal source expanded " << inland_placed << " cells inland" << std::endl;
        return;  // Coastal expansion is complete
    }
    
    // Special handling for Desert: determine hemisphere from source
    bool source_in_northern = IsInNorthernHemisphere(source_y);
    if (def.type == "Desert" && !desert_hemisphere_set_) {
        desert_northern_hemisphere_ = source_in_northern;
        desert_hemisphere_set_ = true;
        std::cout << "Desert source found in " << (desert_northern_hemisphere_ ? "Northern" : "Southern") << " hemisphere" << std::endl;
    }
    
    u32 target_size = random_->RandomU32(def.min_expansion_size, def.max_expansion_size + 1);
    
    // Cap expansion size to prevent exceeding grid capacity
    // Reserve some space for other region types (at least 30% of grid)
    u32 max_grid_cells = static_cast<u32>(grid_width_) * static_cast<u32>(grid_height_);
    u32 max_expansion = max_grid_cells * 7 / 10;  // Max 70% of grid for any single expansion
    if (target_size > max_expansion) {
        target_size = max_expansion;
        std::cout << "Warning: Capped expansion size to " << target_size << " (grid has " << max_grid_cells << " cells)" << std::endl;
    }
    
    u32 placed = 0;
    
    // Use a more efficient expansion: start from placed cells, expand outward
    std::vector<std::pair<u16, u16>> placed_cells;
    std::unordered_set<u32> visited;
    placed_cells.push_back({source_x, source_y});
    visited.insert(static_cast<u32>(source_y) * static_cast<u32>(grid_width_) + static_cast<u32>(source_x));
    
    // Maximum iterations to prevent infinite loops
    const u32 max_iterations = target_size * 100;
    u32 iterations = 0;
    
    // Use a higher base probability - expansion_weight is now a multiplier
    f32 base_expand_prob = 0.3f;  // 30% base chance
    f32 expand_prob = base_expand_prob * def.expansion_weight;
    expand_prob = std::min(1.0f, expand_prob);  // Cap at 100%
    
    while (!placed_cells.empty() && placed < target_size && iterations < max_iterations) {
        iterations++;
        
        // Pick a random placed cell to expand from
        u32 seed_idx = random_->RandomU32(0, static_cast<u32>(placed_cells.size()));
        std::pair<u16, u16> current = placed_cells[seed_idx];
        
        // Try to expand to neighbors
        bool expanded_this_iteration = false;
        std::vector<std::pair<u16, u16>> candidates;
        
        for (i16 dy = -1; dy <= 1; ++dy) {
            for (i16 dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                
                i16 nx = static_cast<i16>(current.first) + dx;
                i16 ny = static_cast<i16>(current.second) + dy;
                
                if (nx < 0 || nx >= static_cast<i16>(grid_width_) ||
                    ny < 0 || ny >= static_cast<i16>(grid_height_)) {
                    continue;
                }
                
                u16 gx = static_cast<u16>(nx);
                u16 gy = static_cast<u16>(ny);
                u32 pos_key = static_cast<u32>(gy) * static_cast<u32>(grid_width_) + static_cast<u32>(gx);
                
                if (visited.find(pos_key) != visited.end()) {
                    continue;
                }
                
                // Special handling for Desert: must stay in hemisphere and avoid rivers
                if (def.type == "Desert" && desert_hemisphere_set_) {
                    bool in_northern = IsInNorthernHemisphere(gy);
                    if (in_northern != desert_northern_hemisphere_) {
                        continue;  // Must stay in same hemisphere
                    }
                    
                    // Check for rivers in neighbors (100% aversion)
                    Region* check_region = GetRegionAtGrid(world, gx, gy);
                    if (check_region && check_region->GetType() == "River") {
                        continue;  // Cannot expand to river cells
                    }
                    
                    // Check neighbors for rivers
                    bool has_river_neighbor = false;
                    for (i16 check_dy = -1; check_dy <= 1 && !has_river_neighbor; ++check_dy) {
                        for (i16 check_dx = -1; check_dx <= 1 && !has_river_neighbor; ++check_dx) {
                            if (check_dx == 0 && check_dy == 0) continue;
                            i16 check_nx = static_cast<i16>(gx) + check_dx;
                            i16 check_ny = static_cast<i16>(gy) + check_dy;
                            if (check_nx >= 0 && check_nx < static_cast<i16>(grid_width_) &&
                                check_ny >= 0 && check_ny < static_cast<i16>(grid_height_)) {
                                Region* neighbor = GetRegionAtGrid(world, static_cast<u16>(check_nx), static_cast<u16>(check_ny));
                                if (neighbor && neighbor->GetType() == "River") {
                                    has_river_neighbor = true;
                                }
                            }
                        }
                    }
                    if (has_river_neighbor) {
                        continue;  // Cannot expand next to rivers
                    }
                }
                
                Region* region = GetRegionAtGrid(world, gx, gy);
                if (!region) {
                    continue;
                }
                
                // Check if we can expand here
                // Allow expansion into Plains, same type, or compatible neighbor types
                bool can_expand_into = false;
                if (region->GetType() == "Plains" || region->GetType() == def.type) {
                    can_expand_into = true;
                } else {
                    // Check if this region type is in our compatible neighbors list
                    for (const auto& compatible_type : def.compatible_neighbors) {
                        if (region->GetType() == compatible_type) {
                            can_expand_into = true;
                            break;
                        }
                    }
                }
                
                if (!can_expand_into) {
                    continue;
                }
                
                // Check compatibility
                if (!CanPlaceRegion(world, gx, gy, def)) {
                    continue;
                }
                
                candidates.push_back({gx, gy});
            }
        }
        
        // Try to place from candidates
        for (const auto& candidate : candidates) {
            if (placed >= target_size) {
                break;
            }
            
            u32 pos_key = static_cast<u32>(candidate.second) * static_cast<u32>(grid_width_) + static_cast<u32>(candidate.first);
            if (visited.find(pos_key) != visited.end()) {
                continue;
            }
            
            // Use probability to decide if we expand here
            if (random_->RandomFloat(0.0f, 1.0f) < expand_prob) {
                Region* region = GetRegionAtGrid(world, candidate.first, candidate.second);
                if (region) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, def.type);
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetSourceParentID(source_id);
                    regions[id]->Initialize();
                    
                    visited.insert(pos_key);
                    placed_cells.push_back(candidate);
                    placed++;
                    expanded_this_iteration = true;
                }
            }
        }
        
        // If we haven't expanded in a while and have candidates, force expansion
        if (!expanded_this_iteration && !candidates.empty() && iterations % 10 == 0) {
            // Force place one candidate
            auto candidate = candidates[random_->RandomU32(0, static_cast<u32>(candidates.size()))];
            u32 pos_key = static_cast<u32>(candidate.second) * static_cast<u32>(grid_width_) + static_cast<u32>(candidate.first);
            
            if (visited.find(pos_key) == visited.end()) {
                Region* region = GetRegionAtGrid(world, candidate.first, candidate.second);
                if (region) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, def.type);
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetSourceParentID(source_id);
                    regions[id]->Initialize();
                    
                    visited.insert(pos_key);
                    placed_cells.push_back(candidate);
                    placed++;
                }
            }
        }
    }
    
    if (placed < target_size && iterations >= max_iterations) {
        std::cout << "Warning: Expansion for source " << source_id << " stopped early. Placed " 
                  << placed << "/" << target_size << " regions after " << iterations << " iterations" << std::endl;
    }
}

std::string StandardWorldGenerator::GetRandomName(const RegionDefinition& def) {
    if (def.potential_names.empty()) {
        return def.type;
    }
    u32 idx = random_->RandomU32(0, static_cast<u32>(def.potential_names.size()));
    return def.potential_names[idx];
}

void StandardWorldGenerator::EnsureCompleteCoastalBorders(World* world) {
    std::cout << "Ensuring complete coastal border coverage..." << std::endl;
    
    // Check which borders have at least one coastal region
    bool top_has_coastal = false;
    bool bottom_has_coastal = false;
    bool left_has_coastal = false;
    bool right_has_coastal = false;
    
    // Check top border
    for (u16 x = 0; x < grid_width_; ++x) {
        Region* region = GetRegionAtGrid(world, x, 0);
        if (region && region->GetType() == "Coastal") {
            top_has_coastal = true;
            break;
        }
    }
    
    // Check bottom border
    for (u16 x = 0; x < grid_width_; ++x) {
        Region* region = GetRegionAtGrid(world, x, grid_height_ - 1);
        if (region && region->GetType() == "Coastal") {
            bottom_has_coastal = true;
            break;
        }
    }
    
    // Check left border
    for (u16 y = 0; y < grid_height_; ++y) {
        Region* region = GetRegionAtGrid(world, 0, y);
        if (region && region->GetType() == "Coastal") {
            left_has_coastal = true;
            break;
        }
    }
    
    // Check right border
    for (u16 y = 0; y < grid_height_; ++y) {
        Region* region = GetRegionAtGrid(world, grid_width_ - 1, y);
        if (region && region->GetType() == "Coastal") {
            right_has_coastal = true;
            break;
        }
    }
    
    // Get coastal region definition for compatibility checks
    auto& config = Config::Configuration::GetInstance();
    const auto& region_definitions = config.regions.region_definitions;
    auto coastal_it = region_definitions.find("Coastal");
    if (coastal_it == region_definitions.end()) {
        std::cout << "Warning: Coastal region definition not found, skipping border enforcement" << std::endl;
        return;
    }
    const RegionDefinition& coastal_def = coastal_it->second;
    
    u32 converted_count = 0;
    
    // Helper function to find a coastal source on a border
    auto FindCoastalSourceOnBorder = [&](u16 border_x, u16 border_y) -> RegionID {
        // Check if this position itself is a coastal source
        Region* check_region = GetRegionAtGrid(world, border_x, border_y);
        if (check_region && check_region->GetType() == "Coastal" && check_region->IsSource()) {
            return check_region->GetID();
        }
        // Search along the border for any coastal source
        if (border_y == 0) {  // Top border
            for (u16 x = 0; x < grid_width_; ++x) {
                Region* r = GetRegionAtGrid(world, x, 0);
                if (r && r->GetType() == "Coastal" && r->IsSource()) {
                    return r->GetID();
                }
            }
        } else if (border_y == grid_height_ - 1) {  // Bottom border
            for (u16 x = 0; x < grid_width_; ++x) {
                Region* r = GetRegionAtGrid(world, x, grid_height_ - 1);
                if (r && r->GetType() == "Coastal" && r->IsSource()) {
                    return r->GetID();
                }
            }
        } else if (border_x == 0) {  // Left border
            for (u16 y = 0; y < grid_height_; ++y) {
                Region* r = GetRegionAtGrid(world, 0, y);
                if (r && r->GetType() == "Coastal" && r->IsSource()) {
                    return r->GetID();
                }
            }
        } else if (border_x == grid_width_ - 1) {  // Right border
            for (u16 y = 0; y < grid_height_; ++y) {
                Region* r = GetRegionAtGrid(world, grid_width_ - 1, y);
                if (r && r->GetType() == "Coastal" && r->IsSource()) {
                    return r->GetID();
                }
            }
        }
        return INVALID_REGION_ID;
    };
    
    // Convert entire top border to coastal if it has any coastal region
    if (top_has_coastal) {
        RegionID border_source = FindCoastalSourceOnBorder(0, 0);
        for (u16 x = 0; x < grid_width_; ++x) {
            Region* region = GetRegionAtGrid(world, x, 0);
            if (region && region->GetType() != "Coastal") {
                // Only convert if not incompatible (like Mountain)
                bool incompatible = false;
                for (const auto& incompatible_type : coastal_def.incompatible_neighbors) {
                    if (region->GetType() == incompatible_type) {
                        incompatible = true;
                        break;
                    }
                }
                if (!incompatible) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    // Preserve source info if it exists
                    bool was_source = region->IsSource();
                    RegionID parent_id = region->GetSourceParentID();
                    
                    // If no parent, use border source if found
                    if (parent_id == INVALID_REGION_ID && border_source != INVALID_REGION_ID) {
                        parent_id = border_source;
                    }
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    if (was_source) {
                        regions[id]->SetIsSource(true);
                    } else if (parent_id != INVALID_REGION_ID) {
                        regions[id]->SetSourceParentID(parent_id);
                    }
                    regions[id]->Initialize();
                    converted_count++;
                }
            }
        }
    }
    
    // Convert entire bottom border to coastal if it has any coastal region
    if (bottom_has_coastal) {
        RegionID border_source = FindCoastalSourceOnBorder(0, grid_height_ - 1);
        for (u16 x = 0; x < grid_width_; ++x) {
            Region* region = GetRegionAtGrid(world, x, grid_height_ - 1);
            if (region && region->GetType() != "Coastal") {
                bool incompatible = false;
                for (const auto& incompatible_type : coastal_def.incompatible_neighbors) {
                    if (region->GetType() == incompatible_type) {
                        incompatible = true;
                        break;
                    }
                }
                if (!incompatible) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    bool was_source = region->IsSource();
                    RegionID parent_id = region->GetSourceParentID();
                    
                    if (parent_id == INVALID_REGION_ID && border_source != INVALID_REGION_ID) {
                        parent_id = border_source;
                    }
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    if (was_source) {
                        regions[id]->SetIsSource(true);
                    } else if (parent_id != INVALID_REGION_ID) {
                        regions[id]->SetSourceParentID(parent_id);
                    }
                    regions[id]->Initialize();
                    converted_count++;
                }
            }
        }
    }
    
    // Convert entire left border to coastal if it has any coastal region
    if (left_has_coastal) {
        RegionID border_source = FindCoastalSourceOnBorder(0, 0);
        for (u16 y = 0; y < grid_height_; ++y) {
            Region* region = GetRegionAtGrid(world, 0, y);
            if (region && region->GetType() != "Coastal") {
                bool incompatible = false;
                for (const auto& incompatible_type : coastal_def.incompatible_neighbors) {
                    if (region->GetType() == incompatible_type) {
                        incompatible = true;
                        break;
                    }
                }
                if (!incompatible) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    bool was_source = region->IsSource();
                    RegionID parent_id = region->GetSourceParentID();
                    
                    if (parent_id == INVALID_REGION_ID && border_source != INVALID_REGION_ID) {
                        parent_id = border_source;
                    }
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    if (was_source) {
                        regions[id]->SetIsSource(true);
                    } else if (parent_id != INVALID_REGION_ID) {
                        regions[id]->SetSourceParentID(parent_id);
                    }
                    regions[id]->Initialize();
                    converted_count++;
                }
            }
        }
    }
    
    // Convert entire right border to coastal if it has any coastal region
    if (right_has_coastal) {
        RegionID border_source = FindCoastalSourceOnBorder(grid_width_ - 1, 0);
        for (u16 y = 0; y < grid_height_; ++y) {
            Region* region = GetRegionAtGrid(world, grid_width_ - 1, y);
            if (region && region->GetType() != "Coastal") {
                bool incompatible = false;
                for (const auto& incompatible_type : coastal_def.incompatible_neighbors) {
                    if (region->GetType() == incompatible_type) {
                        incompatible = true;
                        break;
                    }
                }
                if (!incompatible) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    bool was_source = region->IsSource();
                    RegionID parent_id = region->GetSourceParentID();
                    
                    if (parent_id == INVALID_REGION_ID && border_source != INVALID_REGION_ID) {
                        parent_id = border_source;
                    }
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    if (was_source) {
                        regions[id]->SetIsSource(true);
                    } else if (parent_id != INVALID_REGION_ID) {
                        regions[id]->SetSourceParentID(parent_id);
                    }
                    regions[id]->Initialize();
                    converted_count++;
                }
            }
        }
    }
    
    // Update coastal_borders_ tracking
    if (top_has_coastal) coastal_borders_.insert("top");
    if (bottom_has_coastal) coastal_borders_.insert("bottom");
    if (left_has_coastal) coastal_borders_.insert("left");
    if (right_has_coastal) coastal_borders_.insert("right");
    
    std::cout << "Converted " << converted_count << " border regions to coastal" << std::endl;
}

bool StandardWorldGenerator::CanPlaceRegion(World* world, u16 x, u16 y, const RegionDefinition& def) {
    Region* region = GetRegionAtGrid(world, x, y);
    if (!region) {
        return false;
    }
    
    // Prevent non-coastal regions from being placed on borders that have coastal regions
    if (def.type != "Coastal" && IsOnRim(x, y)) {
        bool on_top = (y == 0);
        bool on_bottom = (y == grid_height_ - 1);
        bool on_left = (x == 0);
        bool on_right = (x == grid_width_ - 1);
        
        // Check if this border is marked as having coastal regions
        if ((on_top && coastal_borders_.find("top") != coastal_borders_.end()) ||
            (on_bottom && coastal_borders_.find("bottom") != coastal_borders_.end()) ||
            (on_left && coastal_borders_.find("left") != coastal_borders_.end()) ||
            (on_right && coastal_borders_.find("right") != coastal_borders_.end())) {
            return false;  // This border has coastal regions, cannot place other types here
        }
        
        // Also check if the current region is already coastal (protect existing coastal regions)
        if (region->GetType() == "Coastal") {
            return false;  // Cannot overwrite existing coastal region on border
        }
    }
    
    // Check if current type is compatible
    // Allow placement on Plains, same type, or compatible neighbor types
    bool can_place_on = false;
    if (region->GetType() == "Plains" || region->GetType() == def.type) {
        can_place_on = true;
    } else {
        // Check if this region type is in our compatible neighbors list
        for (const auto& compatible_type : def.compatible_neighbors) {
            if (region->GetType() == compatible_type) {
                can_place_on = true;
                break;
            }
        }
    }
    
    if (!can_place_on) {
        return false;
    }
    
    // Check neighbors for incompatibility
    for (i16 dy = -1; dy <= 1; ++dy) {
        for (i16 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            i16 nx = static_cast<i16>(x) + dx;
            i16 ny = static_cast<i16>(y) + dy;
            
            if (nx < 0 || nx >= static_cast<i16>(grid_width_) ||
                ny < 0 || ny >= static_cast<i16>(grid_height_)) {
                continue;
            }
            
            Region* neighbor = GetRegionAtGrid(world, static_cast<u16>(nx), static_cast<u16>(ny));
            if (neighbor) {
                std::string neighbor_type = neighbor->GetType();
                for (const auto& incompatible : def.incompatible_neighbors) {
                    if (neighbor_type == incompatible) {
                        return false;
                    }
                }
            }
        }
    }
    
    return true;
}

// Legacy pass methods (kept for fallback and special cases)
void StandardWorldGenerator::Pass1_InitializePlains(World* world) {
    std::cout << "Pass 1: Initializing plains..." << std::endl;
    
    u32 total_regions = static_cast<u32>(grid_width_) * static_cast<u32>(grid_height_);
    auto& regions = world->GetRegions();
    regions.clear();
    regions.reserve(total_regions);
    
    // Initialize all regions as Plains
    for (u32 i = 0; i < total_regions; ++i) {
        u16 x = static_cast<u16>(i % grid_width_);
        u16 y = static_cast<u16>(i / grid_width_);
        f32 world_x = static_cast<f32>(x) * region_size_;
        f32 world_y = static_cast<f32>(y) * region_size_;
        
        auto region = std::make_unique<Region>(i, "Plains");
        region->SetPosition(world_x, world_y);
        region->Initialize();
        regions.push_back(std::move(region));
    }
    
    std::cout << "Pass 1: Initialized " << total_regions << " plains regions" << std::endl;
}

void StandardWorldGenerator::Pass2_DetermineCoastalBorders(World* world) {
    // This is now handled by source-based generation
    // But we keep it for fallback
    std::cout << "Pass 2: Determining coastal borders (legacy method)..." << std::endl;
    
    u32 coastal_count = 0;
    for (u16 y = 0; y < grid_height_; ++y) {
        for (u16 x = 0; x < grid_width_; ++x) {
            if (IsOnRim(x, y)) {
                Region* region = GetRegionAtGrid(world, x, y);
                if (region) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->Initialize();
                    coastal_count++;
                }
            }
        }
    }
    
    std::cout << "Pass 2: Placed " << coastal_count << " coastal regions" << std::endl;
}

void StandardWorldGenerator::Pass3_DetermineMountains(World* world) {
    // Legacy method - now handled by source-based generation
    (void)world;
}

void StandardWorldGenerator::Pass4_DetermineForests(World* world) {
    // Legacy method - now handled by source-based generation
    (void)world;
}

void StandardWorldGenerator::Pass5_DetermineRivers(World* world) {
    std::cout << "Pass 5: Determining rivers..." << std::endl;
    
    // Get config to check for river types in region definitions
    auto& config = Config::Configuration::GetInstance();
    
    // Ensure region definitions are loaded
    if (config.regions.region_definitions.empty()) {
        LoadRegionDefinitions(config.regions);
    }
    
    const auto& region_definitions = config.regions.region_definitions;
    
    bool river_type_exists = (region_definitions.find("River") != region_definitions.end());
    bool river_source_type_exists = (region_definitions.find("RiverSource") != region_definitions.end());
    
    if (!river_type_exists || !river_source_type_exists) {
        std::cout << "Pass 5: Skipped - River or RiverSource types not in config" << std::endl;
        return;
    }
    
    // Find all RiverSource regions (they should have been created as sources)
    std::vector<RegionID> river_sources;
    for (const auto& region : world->GetRegions()) {
        if (region && region->GetType() == "RiverSource" && region->IsSource()) {
            river_sources.push_back(region->GetID());
        }
    }
    
    // If no sources found, create some near mountains
    if (river_sources.empty()) {
        // Find regions adjacent to mountains
        for (u16 y = 0; y < grid_height_; ++y) {
            for (u16 x = 0; x < grid_width_; ++x) {
                Region* region = GetRegionAtGrid(world, x, y);
                if (!region || region->GetType() == "Mountain" || region->GetType() == "Water" || 
                    region->GetType() == "Coastal" || region->GetType() == "River" || region->GetType() == "RiverSource") {
                    continue;
                }
                
                // Check if adjacent to mountain
                bool adjacent_to_mountain = false;
                Region* neighbors[4] = {
                    GetRegionAtGrid(world, x, y - 1),
                    GetRegionAtGrid(world, x, y + 1),
                    GetRegionAtGrid(world, x - 1, y),
                    GetRegionAtGrid(world, x + 1, y)
                };
                
                for (Region* neighbor : neighbors) {
                    if (neighbor && neighbor->GetType() == "Mountain") {
                        adjacent_to_mountain = true;
                        break;
                    }
                }
                
                if (adjacent_to_mountain && random_->RandomFloat(0.0f, 1.0f) < 0.10f) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    regions[id] = std::make_unique<Region>(id, "RiverSource");
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetIsSource(true);
                    
                    // Get name from definition if available
                    auto it = config.regions.region_definitions.find("RiverSource");
                    if (it != config.regions.region_definitions.end()) {
                        regions[id]->SetName(GetRandomName(it->second));
                    } else {
                        regions[id]->SetName("River Source");
                    }
                    
                    regions[id]->Initialize();
                    world->AddSourceRegion(id);
                    river_sources.push_back(id);
                }
            }
        }
    }
    
    // Create rivers from sources to coastal regions
    u32 river_count = 0;
    for (RegionID source_id : river_sources) {
        Region* source_region = world->GetRegion(source_id);
        if (!source_region) continue;
        
        f32 source_wx = source_region->GetX();
        f32 source_wy = source_region->GetY();
        u16 source_x = static_cast<u16>(source_wx / region_size_);
        u16 source_y = static_cast<u16>(source_wy / region_size_);
        
        // Find nearest coastal region
        std::pair<u16, u16> nearest_coastal;
        bool found_coastal = false;
        f32 min_dist = std::numeric_limits<f32>::max();
        
        for (u16 cy = 0; cy < grid_height_; ++cy) {
            for (u16 cx = 0; cx < grid_width_; ++cx) {
                Region* region = GetRegionAtGrid(world, cx, cy);
                if (region && region->GetType() == "Coastal") {
                    f32 dist = std::sqrt(
                        std::pow(static_cast<f32>(source_x) - static_cast<f32>(cx), 2.0f) +
                        std::pow(static_cast<f32>(source_y) - static_cast<f32>(cy), 2.0f)
                    );
                    if (dist < min_dist) {
                        min_dist = dist;
                        nearest_coastal = {cx, cy};
                        found_coastal = true;
                    }
                }
            }
        }
        
        if (!found_coastal) {
            continue;
        }
        
        // Create river path with merging logic
        std::vector<std::pair<u16, u16>> river_path = FindPathWithMerging(world, {source_x, source_y}, nearest_coastal);
        
        // Place river regions along the path
        for (const auto& path_pos : river_path) {
            Region* region = GetRegionAtGrid(world, path_pos.first, path_pos.second);
            if (region && region->GetType() != "Water" && region->GetType() != "Coastal" && 
                region->GetType() != "Mountain" && region->GetType() != "RiverSource") {
                RegionID id = region->GetID();
                f32 rx_world = region->GetX();
                f32 ry_world = region->GetY();
                auto& regions = world->GetRegions();
                regions[id] = std::make_unique<Region>(id, "River");
                regions[id]->SetPosition(rx_world, ry_world);
                regions[id]->SetSourceParentID(source_id);
                regions[id]->Initialize();
                river_count++;
            }
        }
    }
    
    std::cout << "Pass 5: Created " << river_count << " river regions from " << river_sources.size() << " sources" << std::endl;
}

void StandardWorldGenerator::Pass6_DetermineDesert(World* world) {
    // Legacy method - now handled by source-based generation
    (void)world;
}

void StandardWorldGenerator::Pass7_SprinkleFreshWater(World* world) {
    // Legacy method - now handled by source-based generation
    (void)world;
}

void StandardWorldGenerator::Pass8_PlaceSettlements(World* world) {
    std::cout << "Pass 8: Placing settlements..." << std::endl;
    
    std::vector<World::Settlement> settlements;
    
    // Required settlements:
    // 1. One near coast or river
    // 2. One near mountain
    // 3. One in plains
    // 4. One near forest
    // 5. Capital (as central as possible to settlements)
    
    // Find settlement near coast or river
    std::vector<std::pair<u16, u16>> candidates;
    for (u16 y = 0; y < grid_height_; ++y) {
        for (u16 x = 0; x < grid_width_; ++x) {
            Region* region = GetRegionAtGrid(world, x, y);
            if (!region || region->GetType() == "Coastal" || region->GetType() == "River" || 
                region->GetType() == "Water" || region->GetType() == "Mountain") {
                continue;
            }
            
            // Check if adjacent to coast or river
            bool near_water = false;
            Region* neighbors[4] = {
                GetRegionAtGrid(world, x, y - 1),
                GetRegionAtGrid(world, x, y + 1),
                GetRegionAtGrid(world, x - 1, y),
                GetRegionAtGrid(world, x + 1, y)
            };
            
            for (Region* neighbor : neighbors) {
                if (neighbor && (neighbor->GetType() == "Coastal" || neighbor->GetType() == "River")) {
                    near_water = true;
                    break;
                }
            }
            
            if (near_water) {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (!candidates.empty()) {
        auto pos = candidates[random_->RandomU32(0, static_cast<u32>(candidates.size()))];
        Region* region = GetRegionAtGrid(world, pos.first, pos.second);
        if (region) {
            World::Settlement settlement;
            settlement.region_id = region->GetID();
            settlement.type = "City";
            settlement.grid_x = pos.first;
            settlement.grid_y = pos.second;
            settlements.push_back(settlement);
            
            // Change region type to Urban
            RegionID id = region->GetID();
            f32 wx = region->GetX();
            f32 wy = region->GetY();
            auto& regions = world->GetRegions();
            regions[id] = std::make_unique<Region>(id, "Urban");
            regions[id]->SetPosition(wx, wy);
            regions[id]->SetIsSource(true);
            regions[id]->SetName("Port City");
            regions[id]->Initialize();
            world->AddSourceRegion(id);
        }
    }
    
    // Find settlement near mountain
    candidates.clear();
    for (u16 y = 0; y < grid_height_; ++y) {
        for (u16 x = 0; x < grid_width_; ++x) {
            Region* region = GetRegionAtGrid(world, x, y);
            if (!region || region->GetType() == "Mountain" || region->GetType() == "Water" || 
                region->GetType() == "Coastal") {
                continue;
            }
            
            bool near_mountain = false;
            Region* neighbors[4] = {
                GetRegionAtGrid(world, x, y - 1),
                GetRegionAtGrid(world, x, y + 1),
                GetRegionAtGrid(world, x - 1, y),
                GetRegionAtGrid(world, x + 1, y)
            };
            
            for (Region* neighbor : neighbors) {
                if (neighbor && neighbor->GetType() == "Mountain") {
                    near_mountain = true;
                    break;
                }
            }
            
            if (near_mountain) {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (!candidates.empty()) {
        auto pos = candidates[random_->RandomU32(0, static_cast<u32>(candidates.size()))];
        Region* region = GetRegionAtGrid(world, pos.first, pos.second);
        if (region) {
            World::Settlement settlement;
            settlement.region_id = region->GetID();
            settlement.type = "City";
            settlement.grid_x = pos.first;
            settlement.grid_y = pos.second;
            settlements.push_back(settlement);
            
            RegionID id = region->GetID();
            f32 wx = region->GetX();
            f32 wy = region->GetY();
            auto& regions = world->GetRegions();
            regions[id] = std::make_unique<Region>(id, "Urban");
            regions[id]->SetPosition(wx, wy);
            regions[id]->SetIsSource(true);
            regions[id]->SetName("Mountain City");
            regions[id]->Initialize();
            world->AddSourceRegion(id);
        }
    }
    
    // Find settlement in plains
    candidates.clear();
    for (u16 y = 0; y < grid_height_; ++y) {
        for (u16 x = 0; x < grid_width_; ++x) {
            Region* region = GetRegionAtGrid(world, x, y);
            if (region && region->GetType() == "Plains") {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (!candidates.empty()) {
        auto pos = candidates[random_->RandomU32(0, static_cast<u32>(candidates.size()))];
        Region* region = GetRegionAtGrid(world, pos.first, pos.second);
        if (region) {
            World::Settlement settlement;
            settlement.region_id = region->GetID();
            settlement.type = "Village";
            settlement.grid_x = pos.first;
            settlement.grid_y = pos.second;
            settlements.push_back(settlement);
            
            RegionID id = region->GetID();
            f32 wx = region->GetX();
            f32 wy = region->GetY();
            auto& regions = world->GetRegions();
            regions[id] = std::make_unique<Region>(id, "Rural");
            regions[id]->SetPosition(wx, wy);
            regions[id]->SetIsSource(true);
            regions[id]->SetName("Plains Village");
            regions[id]->Initialize();
            world->AddSourceRegion(id);
        }
    }
    
    // Find settlement near forest
    candidates.clear();
    for (u16 y = 0; y < grid_height_; ++y) {
        for (u16 x = 0; x < grid_width_; ++x) {
            Region* region = GetRegionAtGrid(world, x, y);
            if (!region || region->GetType() == "Forest" || region->GetType() == "Water" || 
                region->GetType() == "Coastal") {
                continue;
            }
            
            bool near_forest = false;
            Region* neighbors[4] = {
                GetRegionAtGrid(world, x, y - 1),
                GetRegionAtGrid(world, x, y + 1),
                GetRegionAtGrid(world, x - 1, y),
                GetRegionAtGrid(world, x + 1, y)
            };
            
            for (Region* neighbor : neighbors) {
                if (neighbor && neighbor->GetType() == "Forest") {
                    near_forest = true;
                    break;
                }
            }
            
            if (near_forest) {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (!candidates.empty()) {
        auto pos = candidates[random_->RandomU32(0, static_cast<u32>(candidates.size()))];
        Region* region = GetRegionAtGrid(world, pos.first, pos.second);
        if (region) {
            World::Settlement settlement;
            settlement.region_id = region->GetID();
            settlement.type = "Village";
            settlement.grid_x = pos.first;
            settlement.grid_y = pos.second;
            settlements.push_back(settlement);
            
            RegionID id = region->GetID();
            f32 wx = region->GetX();
            f32 wy = region->GetY();
            auto& regions = world->GetRegions();
            regions[id] = std::make_unique<Region>(id, "Rural");
            regions[id]->SetPosition(wx, wy);
            regions[id]->SetIsSource(true);
            regions[id]->SetName("Forest Village");
            regions[id]->Initialize();
            world->AddSourceRegion(id);
        }
    }
    
    // Place capital as central as possible to settlements
    if (!settlements.empty()) {
        std::vector<std::pair<u16, u16>> settlement_positions;
        for (const auto& settlement : settlements) {
            settlement_positions.push_back({settlement.grid_x, settlement.grid_y});
        }
        
        auto centroid = CalculateCentroid(settlement_positions);
        
        // Find nearest suitable location to centroid
        std::pair<u16, u16> best_pos = centroid;
        f32 best_dist = std::numeric_limits<f32>::max();
        
        // Search in a radius around centroid
        for (i16 dy = -10; dy <= 10; ++dy) {
            for (i16 dx = -10; dx <= 10; ++dx) {
                i16 nx = static_cast<i16>(centroid.first) + dx;
                i16 ny = static_cast<i16>(centroid.second) + dy;
                
                if (nx >= 0 && nx < static_cast<i16>(grid_width_) &&
                    ny >= 0 && ny < static_cast<i16>(grid_height_)) {
                    Region* region = GetRegionAtGrid(world, static_cast<u16>(nx), static_cast<u16>(ny));
                    if (region && region->GetType() != "Water" && region->GetType() != "Mountain" && 
                        region->GetType() != "Coastal") {
                        f32 dist = std::sqrt(
                            std::pow(static_cast<f32>(nx) - static_cast<f32>(centroid.first), 2.0f) +
                            std::pow(static_cast<f32>(ny) - static_cast<f32>(centroid.second), 2.0f)
                        );
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_pos = {static_cast<u16>(nx), static_cast<u16>(ny)};
                        }
                    }
                }
            }
        }
        
        Region* region = GetRegionAtGrid(world, best_pos.first, best_pos.second);
        if (region) {
            World::Settlement settlement;
            settlement.region_id = region->GetID();
            settlement.type = "Capital";
            settlement.grid_x = best_pos.first;
            settlement.grid_y = best_pos.second;
            settlements.push_back(settlement);
            
            RegionID id = region->GetID();
            f32 wx = region->GetX();
            f32 wy = region->GetY();
            auto& regions = world->GetRegions();
            regions[id] = std::make_unique<Region>(id, "Urban");
            regions[id]->SetPosition(wx, wy);
            regions[id]->SetIsSource(true);
            regions[id]->SetName("Capital");
            regions[id]->Initialize();
            world->AddSourceRegion(id);
        }
    }
    
    // Add all settlements to world
    for (const auto& settlement : settlements) {
        world->AddSettlement(settlement);
    }
    
    std::cout << "Pass 8: Placed " << settlements.size() << " settlements" << std::endl;
}

void StandardWorldGenerator::Pass9_PathRoads(World* world) {
    std::cout << "Pass 9: Pathing roads between settlements..." << std::endl;
    
    const auto& settlements = world->GetSettlements();
    if (settlements.size() < 2) {
        std::cout << "Pass 9: Skipped - Not enough settlements" << std::endl;
        return;
    }
    
    // Get config to check for Road type
    auto& config = Config::Configuration::GetInstance();
    
    // Ensure region definitions are loaded
    if (config.regions.region_definitions.empty()) {
        LoadRegionDefinitions(config.regions);
    }
    
    const auto& region_definitions = config.regions.region_definitions;
    
    // Check if Road type exists in region definitions
    bool road_type_exists = (region_definitions.find("Road") != region_definitions.end());
    
    if (!road_type_exists) {
        std::cout << "Pass 9: Skipped - Road type not in region definitions" << std::endl;
        return;
    }
    
    // Path roads between all settlements
    for (size_t i = 0; i < settlements.size(); ++i) {
        for (size_t j = i + 1; j < settlements.size(); ++j) {
            std::pair<u16, u16> start = {settlements[i].grid_x, settlements[i].grid_y};
            std::pair<u16, u16> end = {settlements[j].grid_x, settlements[j].grid_y};
            
            std::vector<std::pair<u16, u16>> path = FindRoadPath(world, start, end);
            
            if (!path.empty()) {
                World::Road road;
                road.from_region = settlements[i].region_id;
                road.to_region = settlements[j].region_id;
                road.path = path;
                world->AddRoad(road);
                
                // Place road regions along path
                for (const auto& path_pos : path) {
                    Region* region = GetRegionAtGrid(world, path_pos.first, path_pos.second);
                    if (region && region->GetType() != "Water" && region->GetType() != "Mountain" && 
                        region->GetType() != "Coastal") {
                        RegionID id = region->GetID();
                        f32 rx_world = region->GetX();
                        f32 ry_world = region->GetY();
                        auto& regions = world->GetRegions();
                        // Change to Road if it's not already a settlement type (Urban/Rural stay as is)
                        // Roads can be placed on: Plains, Forest, Desert, Rural, and Road (to connect existing roads)
                        if (region->GetType() == "Plains" || region->GetType() == "Forest" || 
                            region->GetType() == "Desert" || region->GetType() == "Rural" ||
                            region->GetType() == "Road") {
                            // Save region properties before replacing
                            bool was_source = region->IsSource();
                            std::string region_name = region->GetName();
                            RegionID parent_id = region->GetSourceParentID();
                            
                            regions[id] = std::make_unique<Region>(id, "Road");
                            regions[id]->SetPosition(rx_world, ry_world);
                            // Preserve source info if it exists
                            if (was_source) {
                                regions[id]->SetIsSource(true);
                                regions[id]->SetName(region_name);
                            } else if (parent_id != INVALID_REGION_ID) {
                                regions[id]->SetSourceParentID(parent_id);
                            }
                            regions[id]->Initialize();
                        }
                        // Note: Urban regions are left as-is (roads pass through cities but don't replace them)
                    }
                }
            }
        }
    }
    
    // Path one road to a non-coastal border if one exists
    auto non_coastal_border = FindNearestNonCoastalBorder(world);
    if (non_coastal_border.first != 0xFFFF) {
        // Find nearest settlement to border
        std::pair<u16, u16> nearest_settlement;
        f32 min_dist = std::numeric_limits<f32>::max();
        
        for (const auto& settlement : settlements) {
            f32 dist = std::sqrt(
                std::pow(static_cast<f32>(settlement.grid_x) - static_cast<f32>(non_coastal_border.first), 2.0f) +
                std::pow(static_cast<f32>(settlement.grid_y) - static_cast<f32>(non_coastal_border.second), 2.0f)
            );
            if (dist < min_dist) {
                min_dist = dist;
                nearest_settlement = {settlement.grid_x, settlement.grid_y};
            }
        }
        
        std::vector<std::pair<u16, u16>> path = FindRoadPath(world, nearest_settlement, non_coastal_border);
        if (!path.empty()) {
            World::Road road;
            // Find the border region ID
            Region* border_region = GetRegionAtGrid(world, non_coastal_border.first, non_coastal_border.second);
            road.from_region = border_region ? border_region->GetID() : INVALID_REGION_ID;
            road.to_region = INVALID_REGION_ID;
            road.path = path;
            world->AddRoad(road);
            
            // Place road regions
            for (const auto& path_pos : path) {
                Region* region = GetRegionAtGrid(world, path_pos.first, path_pos.second);
                if (region && region->GetType() != "Water" && region->GetType() != "Mountain" && 
                    region->GetType() != "Coastal") {
                    RegionID id = region->GetID();
                    f32 rx_world = region->GetX();
                    f32 ry_world = region->GetY();
                    auto& regions = world->GetRegions();
                    // Change to Road if it's not already a settlement type
                    if (region->GetType() == "Plains" || region->GetType() == "Forest" || 
                        region->GetType() == "Desert" || region->GetType() == "Rural" ||
                        region->GetType() == "Road") {
                        // Save region properties before replacing
                        bool was_source = region->IsSource();
                        std::string region_name = region->GetName();
                        RegionID parent_id = region->GetSourceParentID();
                        
                        regions[id] = std::make_unique<Region>(id, "Road");
                        regions[id]->SetPosition(rx_world, ry_world);
                        // Preserve source info if it exists
                        if (was_source) {
                            regions[id]->SetIsSource(true);
                            regions[id]->SetName(region_name);
                        } else if (parent_id != INVALID_REGION_ID) {
                            regions[id]->SetSourceParentID(parent_id);
                        }
                        regions[id]->Initialize();
                    }
                    // Note: Urban regions are left as-is
                }
            }
        }
    }
    
    std::cout << "Pass 9: Created " << world->GetRoads().size() << " roads" << std::endl;
}

// Helper methods
Region* StandardWorldGenerator::GetRegionAtGrid(World* world, u16 gx, u16 gy) {
    return world->GetRegionAtGrid(gx, gy);
}

bool StandardWorldGenerator::IsOnRim(u16 x, u16 y) const {
    return (x == 0 || x == grid_width_ - 1 || y == 0 || y == grid_height_ - 1);
}

bool StandardWorldGenerator::IsInNorthernHemisphere(u16 y) const {
    return y < grid_height_ / 2;
}

std::vector<std::pair<u16, u16>> StandardWorldGenerator::FindPath(World* /*world*/, 
                                                                    const std::pair<u16, u16>& start,
                                                                    const std::pair<u16, u16>& end) {
    std::vector<std::pair<u16, u16>> path;
    
    i16 start_x = static_cast<i16>(start.first);
    i16 start_y = static_cast<i16>(start.second);
    i16 end_x = static_cast<i16>(end.first);
    i16 end_y = static_cast<i16>(end.second);
    
    i16 current_x = start_x;
    i16 current_y = start_y;
    std::unordered_set<u32> visited_positions;
    u32 max_path_length = static_cast<u32>(grid_width_ + grid_height_) * 2;
    u32 step_count = 0;
    
    while (step_count < max_path_length) {
        step_count++;
        i16 dx = end_x - current_x;
        i16 dy = end_y - current_y;
        
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
            break;
        }
        
        i16 move_x = 0;
        i16 move_y = 0;
        
        if (std::abs(dx) > std::abs(dy)) {
            move_x = (dx > 0) ? 1 : -1;
        } else {
            move_y = (dy > 0) ? 1 : -1;
        }
        
        // Add some randomness for curvy paths
        if (random_->RandomFloat(0.0f, 1.0f) < 0.15f) {
            if (move_x != 0) {
                move_y = random_->RandomBool(0.5f) ? 1 : -1;
            } else {
                move_x = random_->RandomBool(0.5f) ? 1 : -1;
            }
        }
        
        current_x += move_x;
        current_y += move_y;
        
        // Clamp to bounds
        if (current_x < 0) current_x = 0;
        if (current_x >= static_cast<i16>(grid_width_)) current_x = static_cast<i16>(grid_width_) - 1;
        if (current_y < 0) current_y = 0;
        if (current_y >= static_cast<i16>(grid_height_)) current_y = static_cast<i16>(grid_height_) - 1;
        
        u16 rx = static_cast<u16>(current_x);
        u16 ry = static_cast<u16>(current_y);
        
        u32 pos_key = static_cast<u32>(ry) * static_cast<u32>(grid_width_) + static_cast<u32>(rx);
        if (visited_positions.find(pos_key) != visited_positions.end()) {
            break;  // Loop detected
        }
        
        visited_positions.insert(pos_key);
        
        // Don't add start position to path
        if (rx != start.first || ry != start.second) {
            path.push_back({rx, ry});
        }
    }
    
    return path;
}

std::vector<std::pair<u16, u16>> StandardWorldGenerator::FindPathWithMerging(World* world,
                                                                              const std::pair<u16, u16>& start,
                                                                              const std::pair<u16, u16>& end) {
    std::vector<std::pair<u16, u16>> path;
    
    i16 start_x = static_cast<i16>(start.first);
    i16 start_y = static_cast<i16>(start.second);
    i16 end_x = static_cast<i16>(end.first);
    i16 end_y = static_cast<i16>(end.second);
    
    i16 current_x = start_x;
    i16 current_y = start_y;
    std::unordered_set<u32> visited_positions;
    u32 max_path_length = static_cast<u32>(grid_width_ + grid_height_) * 2;
    u32 step_count = 0;
    const u16 merge_search_radius = 5;  // Search radius for nearby rivers
    
    while (step_count < max_path_length) {
        step_count++;
        i16 dx = end_x - current_x;
        i16 dy = end_y - current_y;
        
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
            break;
        }
        
        i16 move_x = 0;
        i16 move_y = 0;
        
        if (std::abs(dx) > std::abs(dy)) {
            move_x = (dx > 0) ? 1 : -1;
        } else {
            move_y = (dy > 0) ? 1 : -1;
        }
        
        // Check for nearby rivers to merge with (within search radius)
        std::pair<u16, u16> nearest_river;
        bool found_river = false;
        f32 nearest_river_dist = static_cast<f32>(merge_search_radius + 1);
        
        for (i16 check_y = current_y - merge_search_radius; check_y <= current_y + merge_search_radius; ++check_y) {
            for (i16 check_x = current_x - merge_search_radius; check_x <= current_x + merge_search_radius; ++check_x) {
                if (check_x < 0 || check_x >= static_cast<i16>(grid_width_) ||
                    check_y < 0 || check_y >= static_cast<i16>(grid_height_)) {
                    continue;
                }
                
                Region* check_region = GetRegionAtGrid(world, static_cast<u16>(check_x), static_cast<u16>(check_y));
                if (check_region && check_region->GetType() == "River") {
                    // Don't count regions already in our path
                    u32 check_pos_key = static_cast<u32>(check_y) * static_cast<u32>(grid_width_) + static_cast<u32>(check_x);
                    if (visited_positions.find(check_pos_key) != visited_positions.end()) {
                        continue;
                    }
                    
                    f32 dist = std::sqrt(
                        std::pow(static_cast<f32>(check_x - current_x), 2.0f) +
                        std::pow(static_cast<f32>(check_y - current_y), 2.0f)
                    );
                    
                    if (dist < nearest_river_dist && dist > 1.0f) {  // Not too close, not too far
                        nearest_river_dist = dist;
                        nearest_river = {static_cast<u16>(check_x), static_cast<u16>(check_y)};
                        found_river = true;
                    }
                }
            }
        }
        
        // Weight movement: 60% toward target, 40% toward nearby river (if found)
        i16 final_move_x = move_x;
        i16 final_move_y = move_y;
        
        if (found_river && nearest_river_dist < static_cast<f32>(merge_search_radius)) {
            i16 merge_dx = static_cast<i16>(nearest_river.first) - current_x;
            i16 merge_dy = static_cast<i16>(nearest_river.second) - current_y;
            
            // Normalize merge direction
            if (std::abs(merge_dx) > std::abs(merge_dy)) {
                merge_dx = (merge_dx > 0) ? 1 : -1;
                merge_dy = 0;
            } else {
                merge_dx = 0;
                merge_dy = (merge_dy > 0) ? 1 : -1;
            }
            
            // 40% chance to move toward nearby river instead of directly toward target
            if (random_->RandomFloat(0.0f, 1.0f) < 0.40f) {
                final_move_x = merge_dx;
                final_move_y = merge_dy;
            } else {
                // Still move toward target, but add slight curve toward river
                if (merge_dx != 0 && move_x == 0) {
                    final_move_x = merge_dx;
                }
                if (merge_dy != 0 && move_y == 0) {
                    final_move_y = merge_dy;
                }
            }
        } else {
            // No nearby river found, use normal curvyness (15% chance to deviate)
            if (random_->RandomFloat(0.0f, 1.0f) < 0.15f) {
                // Perpendicular offset for slight curvyness
                if (move_x != 0) {
                    final_move_y = random_->RandomBool(0.5f) ? 1 : -1;
                } else {
                    final_move_x = random_->RandomBool(0.5f) ? 1 : -1;
                }
            }
        }
        
        current_x += final_move_x;
        current_y += final_move_y;
        
        // Clamp to bounds
        if (current_x < 0) current_x = 0;
        if (current_x >= static_cast<i16>(grid_width_)) current_x = static_cast<i16>(grid_width_) - 1;
        if (current_y < 0) current_y = 0;
        if (current_y >= static_cast<i16>(grid_height_)) current_y = static_cast<i16>(grid_height_) - 1;
        
        u16 rx = static_cast<u16>(current_x);
        u16 ry = static_cast<u16>(current_y);
        
        u32 pos_key = static_cast<u32>(ry) * static_cast<u32>(grid_width_) + static_cast<u32>(rx);
        if (visited_positions.find(pos_key) != visited_positions.end()) {
            break;  // Loop detected
        }
        
        visited_positions.insert(pos_key);
        
        // Don't add start position to path
        if (rx != start.first || ry != start.second) {
            path.push_back({rx, ry});
        }
    }
    
    return path;
}

std::pair<u16, u16> StandardWorldGenerator::FindNearestWaterSource(World* world, u16 x, u16 y) {
    std::pair<u16, u16> nearest = {0xFFFF, 0xFFFF};
    f32 min_dist = std::numeric_limits<f32>::max();
    
    for (u16 cy = 0; cy < grid_height_; ++cy) {
        for (u16 cx = 0; cx < grid_width_; ++cx) {
            Region* region = GetRegionAtGrid(world, cx, cy);
            if (region && (region->GetType() == "Coastal" || region->GetType() == "River" || 
                           region->GetType() == "Water")) {
                f32 dist = std::sqrt(
                    std::pow(static_cast<f32>(x) - static_cast<f32>(cx), 2.0f) +
                    std::pow(static_cast<f32>(y) - static_cast<f32>(cy), 2.0f)
                );
                if (dist < min_dist) {
                    min_dist = dist;
                    nearest = {cx, cy};
                }
            }
        }
    }
    
    return nearest;
}

std::pair<u16, u16> StandardWorldGenerator::FindNearestNonCoastalBorder(World* world) {
    // Find a border region that is not coastal
    for (u16 y = 0; y < grid_height_; ++y) {
        for (u16 x = 0; x < grid_width_; ++x) {
            if (IsOnRim(x, y)) {
                Region* region = GetRegionAtGrid(world, x, y);
                if (region && region->GetType() != "Coastal") {
                    return {x, y};
                }
            }
        }
    }
    return {0xFFFF, 0xFFFF};
}

std::pair<u16, u16> StandardWorldGenerator::CalculateCentroid(const std::vector<std::pair<u16, u16>>& positions) {
    if (positions.empty()) {
        return {0, 0};
    }
    
    u32 sum_x = 0;
    u32 sum_y = 0;
    for (const auto& pos : positions) {
        sum_x += pos.first;
        sum_y += pos.second;
    }
    
    u16 centroid_x = static_cast<u16>(sum_x / positions.size());
    u16 centroid_y = static_cast<u16>(sum_y / positions.size());
    
    return {centroid_x, centroid_y};
}

std::vector<std::pair<u16, u16>> StandardWorldGenerator::FindRoadPath(World* world,
                                                                       const std::pair<u16, u16>& start,
                                                                       const std::pair<u16, u16>& end) {
    // Simple A* pathfinding with terrain costs
    // Roads prefer: Plains, Urban, Rural, Road (cost 1)
    // Roads avoid: Forest, Mountain, River (cost 10, but still allowed)
    // Roads block: Water, Coastal (cost 1000, only if absolutely necessary)
    
    struct Node {
        u16 x, y;
        u32 g_cost;  // Cost from start
        u32 h_cost;  // Heuristic to end
        u32 f_cost() const { return g_cost + h_cost; }
        std::pair<u16, u16> parent;
        
        bool operator<(const Node& other) const {
            return f_cost() > other.f_cost();  // For priority queue (min-heap)
        }
    };
    
    // Get terrain cost for a region type
    auto GetTerrainCost = [](const std::string& region_type) -> u32 {
        if (region_type == "Plains" || region_type == "Urban" || region_type == "Rural" || region_type == "Road") {
            return 1;  // Preferred terrain
        } else if (region_type == "Forest" || region_type == "Mountain" || region_type == "River") {
            return 10;  // Avoided terrain, but allowed
        } else if (region_type == "Water" || region_type == "Coastal") {
            return 1000;  // Very expensive, only if absolutely necessary
        } else {
            return 5;  // Default cost for other types (Desert, etc.)
        }
    };
    
    // Heuristic: Manhattan distance
    auto Heuristic = [](u16 x1, u16 y1, u16 x2, u16 y2) -> u32 {
        return static_cast<u32>(std::abs(static_cast<i32>(x1) - static_cast<i32>(x2)) + 
                                std::abs(static_cast<i32>(y1) - static_cast<i32>(y2)));
    };
    
    std::priority_queue<Node> open_set;
    std::unordered_map<u32, Node> all_nodes;  // pos_key -> Node
    std::unordered_set<u32> closed_set;
    
    // Initialize start node
    Node start_node;
    start_node.x = start.first;
    start_node.y = start.second;
    start_node.g_cost = 0;
    start_node.h_cost = Heuristic(start.first, start.second, end.first, end.second);
    start_node.parent = {0xFFFF, 0xFFFF};
    
    u32 start_key = static_cast<u32>(start.second) * static_cast<u32>(grid_width_) + static_cast<u32>(start.first);
    all_nodes[start_key] = start_node;
    open_set.push(start_node);
    
    const i16 dx[] = {0, 1, 0, -1};  // 4-directional movement
    const i16 dy[] = {-1, 0, 1, 0};
    
    while (!open_set.empty()) {
        Node current = open_set.top();
        open_set.pop();
        
        u32 current_key = static_cast<u32>(current.y) * static_cast<u32>(grid_width_) + static_cast<u32>(current.x);
        
        if (closed_set.find(current_key) != closed_set.end()) {
            continue;  // Already processed
        }
        
        closed_set.insert(current_key);
        
        // Check if we reached the end
        if (current.x == end.first && current.y == end.second) {
            // Reconstruct path
            std::vector<std::pair<u16, u16>> path;
            Node* node = &all_nodes[current_key];
            while (node->parent.first != 0xFFFF) {
                path.push_back({node->x, node->y});
                u32 parent_key = static_cast<u32>(node->parent.second) * static_cast<u32>(grid_width_) + 
                                static_cast<u32>(node->parent.first);
                auto it = all_nodes.find(parent_key);
                if (it == all_nodes.end()) break;
                node = &it->second;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        // Check neighbors
        for (int i = 0; i < 4; ++i) {
            i16 nx = static_cast<i16>(current.x) + dx[i];
            i16 ny = static_cast<i16>(current.y) + dy[i];
            
            if (nx < 0 || nx >= static_cast<i16>(grid_width_) ||
                ny < 0 || ny >= static_cast<i16>(grid_height_)) {
                continue;
            }
            
            u16 gx = static_cast<u16>(nx);
            u16 gy = static_cast<u16>(ny);
            u32 neighbor_key = static_cast<u32>(gy) * static_cast<u32>(grid_width_) + static_cast<u32>(gx);
            
            if (closed_set.find(neighbor_key) != closed_set.end()) {
                continue;  // Already processed
            }
            
            // Get region and calculate cost
            Region* region = GetRegionAtGrid(world, gx, gy);
            if (!region) {
                continue;
            }
            
            std::string region_type = region->GetType();
            if (region_type.empty()) {
                continue;  // Skip invalid regions
            }
            
            u32 terrain_cost = GetTerrainCost(region_type);
            u32 new_g_cost = current.g_cost + terrain_cost;
            
            // Check if we've seen this node before
            auto it = all_nodes.find(neighbor_key);
            if (it != all_nodes.end()) {
                // Update if we found a better path
                if (new_g_cost < it->second.g_cost) {
                    it->second.g_cost = new_g_cost;
                    it->second.parent = {current.x, current.y};
                    open_set.push(it->second);
                }
            } else {
                // New node
                Node neighbor;
                neighbor.x = gx;
                neighbor.y = gy;
                neighbor.g_cost = new_g_cost;
                neighbor.h_cost = Heuristic(gx, gy, end.first, end.second);
                neighbor.parent = {current.x, current.y};
                all_nodes[neighbor_key] = neighbor;
                open_set.push(neighbor);
            }
        }
    }
    
    // No path found, return empty
    return std::vector<std::pair<u16, u16>>();
}

} // namespace Simulation

