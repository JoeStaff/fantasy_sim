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
    
    // Reset state for new generation
    desert_hemisphere_set_ = false;
    desert_northern_hemisphere_ = false;
    forest_hemisphere_set_ = false;
    forest_northern_hemisphere_ = false;
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
        std::cout << "StandardWorldGenerator: WARNING - No region definitions loaded!" << std::endl;
        return world;
    }
    
    // Pass 0: Initialize all regions as Plains
    Pass0_InitializePlains(world.get());
    
    // Special pass: Coastal (must happen first, before other regions)
    auto coastal_it = region_definitions.find("Coastal");
    if (coastal_it != region_definitions.end() && coastal_it->second.spawn_weight > 0.0f) {
        Pass_Coastal(world.get(), coastal_it->second);
    }
    
    // Determine generation order based on dependencies and spawn weights
    std::vector<std::string> generation_order = DetermineGenerationOrder(region_definitions);
    
    // Execute passes: create sources, then expand for each region type
    // (Coastal is excluded from this loop as it's handled separately)
    for (const std::string& region_type : generation_order) {
        // Skip Coastal - already handled
        if (region_type == "Coastal") {
            continue;
        }
        
        auto it = region_definitions.find(region_type);
        if (it == region_definitions.end()) {
            continue;
        }
        
        const RegionDefinition& def = it->second;
        
        // Skip if spawn_weight is 0 (disabled) or no sources needed
        if (def.spawn_weight <= 0.0f || def.max_source_count == 0) {
            continue;
        }
        
        std::cout << "\n=== Pass: " << region_type << " ===" << std::endl;
        
        // Step 1: Create source regions for this type
        std::vector<RegionID> sources_created = Pass_CreateSources(world.get(), region_type, def);
        
        // Step 2: Expand from each source
        if (!sources_created.empty() && def.max_expansion_size > 0) {
            std::cout << "Expanding " << sources_created.size() << " " << region_type << " sources..." << std::endl;
            for (RegionID source_id : sources_created) {
                Pass_ExpandFromSource(world.get(), source_id, def);
            }
            std::cout << "Finished expanding " << region_type << " sources" << std::endl;
        }
    }
    
    // Special passes for regions that don't follow standard source/expand pattern
    Pass_Rivers(world.get(), region_definitions);
    Pass_Settlements(world.get(), region_definitions);
    Pass_Roads(world.get(), region_definitions);
    
    std::cout << "\nStandardWorldGenerator: World generation complete" << std::endl;
    std::cout << "StandardWorldGenerator: Created " << world->GetRegions().size() << " regions" << std::endl;
    std::cout << "StandardWorldGenerator: Created " << world->GetSourceRegions().size() << " source regions" << std::endl;
    std::cout << "StandardWorldGenerator: Created " << world->GetSettlements().size() << " settlements" << std::endl;
    std::cout << "StandardWorldGenerator: Created " << world->GetRoads().size() << " roads" << std::endl;
    
    return world;
}

void StandardWorldGenerator::Pass0_InitializePlains(World* world) {
    std::cout << "Pass 0: Initializing plains..." << std::endl;
    
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
    
    std::cout << "Pass 0: Initialized " << total_regions << " plains regions" << std::endl;
}

std::vector<std::string> StandardWorldGenerator::DetermineGenerationOrder(
    const std::unordered_map<std::string, RegionDefinition>& region_definitions) {
    
    // Define generation order based on dependencies
    // Regions that affect others should come first
    // Note: Coastal is handled separately, not included here
    std::vector<std::string> order = {
        "Mountain",     // Foundation features
        "Forest",       // Large biomes
        "Desert",       // Large biomes
        "Water",        // Smaller features
        "Woods",        // Smaller forest patches
    };
    
    // Filter to only include types that exist in definitions and have spawn_weight > 0
    std::vector<std::string> filtered_order;
    for (const std::string& type : order) {
        auto it = region_definitions.find(type);
        if (it != region_definitions.end() && 
            it->second.spawn_weight > 0.0f && 
            it->second.max_source_count > 0) {
            filtered_order.push_back(type);
        }
    }
    
    return filtered_order;
}

std::vector<RegionID> StandardWorldGenerator::Pass_CreateSources(
    World* world, 
    const std::string& region_type, 
    const RegionDefinition& def) {
    
    std::vector<RegionID> created_sources;
    
    std::cout << "Creating source regions for type: " << region_type << std::endl;
    
    // Use values from region definition
    u32 source_count = random_->RandomU32(def.min_source_count, def.max_source_count + 1);
    
    // Special handling for Desert/Forest: keep them in opposite hemispheres
    if (region_type == "Desert" && !desert_hemisphere_set_) {
        if (forest_hemisphere_set_) {
            desert_northern_hemisphere_ = !forest_northern_hemisphere_;
        } else {
            desert_northern_hemisphere_ = random_->RandomBool(0.5f);
        }
        desert_hemisphere_set_ = true;
        std::cout << "  Desert will be placed in " << (desert_northern_hemisphere_ ? "Northern" : "Southern") << " hemisphere" << std::endl;
    } else if (region_type == "Forest" && !forest_hemisphere_set_) {
        if (desert_hemisphere_set_) {
            forest_northern_hemisphere_ = !desert_northern_hemisphere_;
        } else {
            forest_northern_hemisphere_ = random_->RandomBool(0.5f);
        }
        forest_hemisphere_set_ = true;
        std::cout << "  Forest will be placed in " << (forest_northern_hemisphere_ ? "Northern" : "Southern") << " hemisphere" << std::endl;
    }
    
    u32 placed = 0;
    u32 attempts = 0;
    const u32 max_attempts = source_count * 200;
    
    while (placed < source_count && attempts < max_attempts) {
        attempts++;
        
        // Find a suitable location
        u16 x = static_cast<u16>(random_->RandomU32(0, grid_width_));
        u16 y = static_cast<u16>(random_->RandomU32(0, grid_height_));
        
        // Special handling for Desert - must be in selected hemisphere
        if (region_type == "Desert" && desert_hemisphere_set_) {
            bool in_northern = IsInNorthernHemisphere(y);
            if (in_northern != desert_northern_hemisphere_) {
                continue;
            }
        }
        
        // Special handling for Forest - must be in selected hemisphere
        if (region_type == "Forest" && forest_hemisphere_set_) {
            bool in_northern = IsInNorthernHemisphere(y);
            if (in_northern != forest_northern_hemisphere_) {
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
        
        world->AddSourceRegion(id);
        created_sources.push_back(id);
        placed++;
    }
    
    std::cout << "  Created " << placed << "/" << source_count << " source regions for type: " << region_type << std::endl;
    
    return created_sources;
}

void StandardWorldGenerator::Pass_ExpandFromSource(
    World* world, 
    RegionID source_id, 
    const RegionDefinition& def) {
    
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
    
    // Special handling for coastal regions: expand inland from border
    if (def.type == "Coastal" && IsOnRim(source_x, source_y)) {
        ExpandCoastalInland(world, source_id, def, source_x, source_y);
    } else {
        // Standard expansion for other region types
        ExpandStandardRegion(world, source_id, def, source_x, source_y);
    }
}

void StandardWorldGenerator::ExpandCoastalInland(
    World* world,
    RegionID source_id,
    const RegionDefinition& def,
    u16 source_x,
    u16 source_y) {
    
    // Determine expansion amount (random within min/max)
    u32 target_size = random_->RandomU32(def.min_expansion_size, def.max_expansion_size + 1);
    
    u32 placed = 0;
    std::vector<std::pair<u16, u16>> placed_cells;
    std::unordered_set<u32> visited;
    placed_cells.push_back({source_x, source_y});
    visited.insert(static_cast<u32>(source_y) * static_cast<u32>(grid_width_) + static_cast<u32>(source_x));
    
    const u32 max_iterations = target_size * 100;
    u32 iterations = 0;
    
    f32 base_expand_prob = 0.4f;
    f32 expand_prob = base_expand_prob * def.expansion_weight;
    expand_prob = std::min(1.0f, expand_prob);
    
    while (!placed_cells.empty() && placed < target_size && iterations < max_iterations) {
        iterations++;
        
        u32 seed_idx = random_->RandomU32(0, static_cast<u32>(placed_cells.size()));
        std::pair<u16, u16> current = placed_cells[seed_idx];
        
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
                
                // Don't expand to other borders
                if (IsOnRim(gx, gy)) {
                    continue;
                }
                
                Region* region = GetRegionAtGrid(world, gx, gy);
                if (!region) {
                    continue;
                }
                
                if (region->GetType() != "Plains" && region->GetType() != "Coastal") {
                    continue;
                }
                
                if (!CanPlaceRegion(world, gx, gy, def)) {
                    continue;
                }
                
                candidates.push_back({gx, gy});
            }
        }
        
        for (const auto& candidate : candidates) {
            if (placed >= target_size) {
                break;
            }
            
            u32 pos_key = static_cast<u32>(candidate.second) * static_cast<u32>(grid_width_) + static_cast<u32>(candidate.first);
            if (visited.find(pos_key) != visited.end()) {
                continue;
            }
            
            // Distance-based probability (closer to border = higher chance)
            u16 candidate_dist = std::min({
                candidate.second,
                static_cast<u16>(grid_height_ - 1 - candidate.second),
                candidate.first,
                static_cast<u16>(grid_width_ - 1 - candidate.first)
            });
            
            // Probability decreases with distance from border
            f32 distance_factor = 1.0f - (static_cast<f32>(candidate_dist) / static_cast<f32>(target_size + 5));
            distance_factor = std::max(0.1f, distance_factor);
            
            // Count coastal neighbors
            u32 coastal_neighbor_count = 0;
            for (i16 ndy = -1; ndy <= 1; ++ndy) {
                for (i16 ndx = -1; ndx <= 1; ++ndx) {
                    if (ndx == 0 && ndy == 0) continue;
                    i16 nnx = static_cast<i16>(candidate.first) + ndx;
                    i16 nny = static_cast<i16>(candidate.second) + ndy;
                    if (nnx < 0 || nnx >= static_cast<i16>(grid_width_) ||
                        nny < 0 || nny >= static_cast<i16>(grid_height_)) {
                        continue;
                    }
                    Region* n = GetRegionAtGrid(world, static_cast<u16>(nnx), static_cast<u16>(nny));
                    if (n && n->GetType() == "Coastal") {
                        ++coastal_neighbor_count;
                    }
                }
            }
            
            f32 neighbor_boost = 1.0f + std::min<f32>(3.0f, 0.5f * static_cast<f32>(coastal_neighbor_count));
            
            f32 effective_weight = def.expansion_weight;
            if (effective_weight < 0.3f) {
                effective_weight = 0.3f;
            }
            
            f32 final_expand_prob = expand_prob * distance_factor * neighbor_boost;
            final_expand_prob = std::min(1.0f, final_expand_prob);
            
            if (random_->RandomFloat(0.0f, 1.0f) < final_expand_prob) {
                Region* region = GetRegionAtGrid(world, candidate.first, candidate.second);
                if (region) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetSourceParentID(source_id);
                    regions[id]->Initialize();
                    
                    visited.insert(pos_key);
                    placed_cells.push_back(candidate);
                    placed++;
                }
            }
        }
        
        // Force expansion if stuck
        if (placed == 0 && !candidates.empty() && iterations % 10 == 0) {
            auto forced = candidates[random_->RandomU32(0, static_cast<u32>(candidates.size()))];
            u32 pos_key = static_cast<u32>(forced.second) * static_cast<u32>(grid_width_) + static_cast<u32>(forced.first);
            
            if (visited.find(pos_key) == visited.end()) {
                Region* region = GetRegionAtGrid(world, forced.first, forced.second);
                if (region) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetSourceParentID(source_id);
                    regions[id]->Initialize();
                    
                    visited.insert(pos_key);
                    placed_cells.push_back(forced);
                    placed++;
                }
            }
        }
    }
    
    if (placed < target_size && iterations >= max_iterations) {
        std::cout << "Warning: Coastal expansion for source " << source_id << " stopped early. Placed " 
                  << placed << "/" << target_size << " regions after " << iterations << " iterations" << std::endl;
    }
}

void StandardWorldGenerator::Pass_Coastal(World* world, const RegionDefinition& def) {
    std::cout << "\n=== Pass: Coastal ===" << std::endl;
    
    // Step 1: Randomly select 0-4 borders to be coasts
    std::vector<std::string> all_borders = {"top", "bottom", "left", "right"};
    std::vector<std::string> selected_borders;
    
    u32 border_count = random_->RandomU32(0, 4);  // 0-4 borders
    std::vector<std::string> available_borders = all_borders;
    
    for (u32 i = 0; i < border_count && !available_borders.empty(); ++i) {
        // RandomU32 is inclusive [min, max], so we need size-1 to avoid out-of-bounds
        u32 size = static_cast<u32>(available_borders.size());
        if (size == 0) break;  // Safety check (shouldn't happen due to loop condition)
        u32 idx = random_->RandomU32(0, size - 1);
        selected_borders.push_back(available_borders[idx]);
        available_borders.erase(available_borders.begin() + idx);
    }
    
    std::cout << "Selected " << selected_borders.size() << " border(s) for coastal generation: ";
    for (const auto& border : selected_borders) {
        std::cout << border << " ";
    }
    std::cout << std::endl;
    
    if (selected_borders.empty()) {
        std::cout << "No borders selected for coastal generation" << std::endl;
        return;
    }
    
    // Track which borders have coastal regions
    for (const auto& border : selected_borders) {
        coastal_borders_.insert(border);
    }
    
    // Step 2: Convert all regions along selected borders to Coastal (all as sources)
    std::vector<RegionID> source_regions;
    
    for (const auto& border : selected_borders) {
        if (border == "top") {
            for (u16 x = 0; x < grid_width_; ++x) {
                Region* region = GetRegionAtGrid(world, x, 0);
                if (region && CanPlaceRegion(world, x, 0, def)) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetIsSource(true);
                    regions[id]->SetName(GetRandomName(def));
                    regions[id]->Initialize();
                    
                    world->AddSourceRegion(id);
                    source_regions.push_back(id);
                }
            }
        } else if (border == "bottom") {
            for (u16 x = 0; x < grid_width_; ++x) {
                Region* region = GetRegionAtGrid(world, x, grid_height_ - 1);
                if (region && CanPlaceRegion(world, x, grid_height_ - 1, def)) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetIsSource(true);
                    regions[id]->SetName(GetRandomName(def));
                    regions[id]->Initialize();
                    
                    world->AddSourceRegion(id);
                    source_regions.push_back(id);
                }
            }
        } else if (border == "left") {
            for (u16 y = 0; y < grid_height_; ++y) {
                Region* region = GetRegionAtGrid(world, 0, y);
                if (region && CanPlaceRegion(world, 0, y, def)) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetIsSource(true);
                    regions[id]->SetName(GetRandomName(def));
                    regions[id]->Initialize();
                    
                    world->AddSourceRegion(id);
                    source_regions.push_back(id);
                }
            }
        } else if (border == "right") {
            for (u16 y = 0; y < grid_height_; ++y) {
                Region* region = GetRegionAtGrid(world, grid_width_ - 1, y);
                if (region && CanPlaceRegion(world, grid_width_ - 1, y, def)) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    auto& regions = world->GetRegions();
                    
                    regions[id] = std::make_unique<Region>(id, "Coastal");
                    regions[id]->SetPosition(wx, wy);
                    regions[id]->SetIsSource(true);
                    regions[id]->SetName(GetRandomName(def));
                    regions[id]->Initialize();
                    
                    world->AddSourceRegion(id);
                    source_regions.push_back(id);
                }
            }
        }
    }
    
    std::cout << "Converted " << source_regions.size() << " border cells to Coastal (all as sources)" << std::endl;
    
    // Step 3: Expand from each source independently
    if (def.max_expansion_size > 0 && !source_regions.empty()) {
        std::cout << "Expanding " << source_regions.size() << " coastal sources inland..." << std::endl;
        
        for (RegionID source_id : source_regions) {
            Pass_ExpandFromSource(world, source_id, def);
        }
        
        std::cout << "Finished expanding coastal sources" << std::endl;
    }
}

void StandardWorldGenerator::ExpandStandardRegion(
    World* world,
    RegionID source_id,
    const RegionDefinition& def,
    u16 source_x,
    u16 source_y) {
    
    Region* source_region = world->GetRegion(source_id);
    if (!source_region) {
        return;
    }
    
    // Special handling for Desert/Forest: determine hemisphere from first source
    bool source_in_northern = IsInNorthernHemisphere(source_y);
    if (def.type == "Desert") {
        if (!desert_hemisphere_set_) {
            desert_northern_hemisphere_ = source_in_northern;
            desert_hemisphere_set_ = true;
        }
        if (forest_hemisphere_set_) {
            forest_northern_hemisphere_ = !desert_northern_hemisphere_;
        }
    } else if (def.type == "Forest") {
        if (!forest_hemisphere_set_) {
            forest_northern_hemisphere_ = source_in_northern;
            forest_hemisphere_set_ = true;
        }
        if (desert_hemisphere_set_) {
            desert_northern_hemisphere_ = !forest_northern_hemisphere_;
        }
    }
    
    u32 target_size = random_->RandomU32(def.min_expansion_size, def.max_expansion_size + 1);
    
    // Cap expansion size to prevent exceeding grid capacity
    u32 max_grid_cells = static_cast<u32>(grid_width_) * static_cast<u32>(grid_height_);
    u32 max_expansion = max_grid_cells * 7 / 10;
    if (target_size > max_expansion) {
        target_size = max_expansion;
    }
    
    u32 placed = 0;
    
    std::vector<std::pair<u16, u16>> placed_cells;
    std::unordered_set<u32> visited;
    placed_cells.push_back({source_x, source_y});
    visited.insert(static_cast<u32>(source_y) * static_cast<u32>(grid_width_) + static_cast<u32>(source_x));
    
    const u32 max_iterations = target_size * 100;
    u32 iterations = 0;
    
    f32 base_expand_prob = 0.3f;
    f32 expand_prob = base_expand_prob * def.expansion_weight;
    expand_prob = std::min(1.0f, expand_prob);
    
    while (!placed_cells.empty() && placed < target_size && iterations < max_iterations) {
        iterations++;
        
        u32 seed_idx = random_->RandomU32(0, static_cast<u32>(placed_cells.size()));
        std::pair<u16, u16> current = placed_cells[seed_idx];
        
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
                        continue;
                    }
                    
                    Region* check_region = GetRegionAtGrid(world, gx, gy);
                    if (check_region && check_region->GetType() == "River") {
                        continue;
                    }
                    
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
                        continue;
                    }
                }
                
                // Special handling for Forest: must stay in its selected hemisphere
                if (def.type == "Forest" && forest_hemisphere_set_) {
                    bool in_northern_forest = IsInNorthernHemisphere(gy);
                    if (in_northern_forest != forest_northern_hemisphere_) {
                        continue;
                    }
                }
                
                Region* region = GetRegionAtGrid(world, gx, gy);
                if (!region) {
                    continue;
                }
                
                bool can_expand_into = false;
                if (region->GetType() == "Plains" || region->GetType() == def.type) {
                    can_expand_into = true;
                } else {
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
                
                if (!CanPlaceRegion(world, gx, gy, def)) {
                    continue;
                }
                
                candidates.push_back({gx, gy});
            }
        }
        
        for (const auto& candidate : candidates) {
            if (placed >= target_size) {
                break;
            }
            
            u32 pos_key = static_cast<u32>(candidate.second) * static_cast<u32>(grid_width_) + static_cast<u32>(candidate.first);
            if (visited.find(pos_key) != visited.end()) {
                continue;
            }
            
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
        
        if (!expanded_this_iteration && !candidates.empty() && iterations % 10 == 0) {
            auto candidate = candidates[random_->RandomU32(0, static_cast<u32>(candidates.size()))];
            u32 pos_key = static_cast<u32>(candidate.second) * static_cast<u32>(grid_width_) + static_cast<u32>(candidate.first);
            
            if (visited.find(pos_key) == visited.end()) {
                Region* region = GetRegionAtGrid(world, candidate.first, candidate.second);
                if (region) {
                    // Special check for Desert: never overwrite rivers
                    if (def.type == "Desert" && region->GetType() == "River") {
                        continue;
                    }
                    
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

void StandardWorldGenerator::Pass_Rivers(
    World* world,
    const std::unordered_map<std::string, RegionDefinition>& region_definitions) {
    
    std::cout << "\n=== Pass: Rivers ===" << std::endl;
    
    auto river_source_it = region_definitions.find("RiverSource");
    auto river_it = region_definitions.find("River");
    
    if (river_source_it == region_definitions.end() || river_it == region_definitions.end()) {
        std::cout << "Pass_Rivers: Skipped - River or RiverSource types not in config" << std::endl;
        return;
    }
    
    const RegionDefinition& river_source_def = river_source_it->second;
    const RegionDefinition& river_def = river_it->second;
    
    // Step 1: Create RiverSource regions near mountains
    std::vector<RegionID> river_sources = Pass_CreateRiverSources(world, river_source_def);
    
    // Step 2: Expand from each RiverSource to create River paths to coastal regions
    if (!river_sources.empty()) {
        std::cout << "Creating rivers from " << river_sources.size() << " sources..." << std::endl;
        for (RegionID source_id : river_sources) {
            Pass_ExpandRiverFromSource(world, source_id, river_def);
        }
    }
}

std::vector<RegionID> StandardWorldGenerator::Pass_CreateRiverSources(
    World* world,
    const RegionDefinition& def) {
    
    std::vector<RegionID> created_sources;
    
    // Find regions adjacent to mountains
    std::vector<std::pair<u16, u16>> candidates;
    for (u16 y = 0; y < grid_height_; ++y) {
        for (u16 x = 0; x < grid_width_; ++x) {
            Region* region = GetRegionAtGrid(world, x, y);
            if (!region || region->GetType() == "Mountain" || region->GetType() == "Water" || 
                region->GetType() == "Coastal" || region->GetType() == "River" || region->GetType() == "RiverSource") {
                continue;
            }
            
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
            
            if (adjacent_to_mountain && CanPlaceRegion(world, x, y, def)) {
                candidates.push_back({x, y});
            }
        }
    }
    
    // Create 1-3 river sources
    u32 source_count = random_->RandomU32(1, 4);
    source_count = std::min(source_count, static_cast<u32>(candidates.size()));
    
    // Shuffle candidates
    for (u32 i = 0; i < candidates.size() && created_sources.size() < source_count; ++i) {
        u32 j = random_->RandomU32(i, static_cast<u32>(candidates.size()));
        std::swap(candidates[i], candidates[j]);
        
        auto& pos = candidates[i];
        if (random_->RandomFloat(0.0f, 1.0f) < 0.10f) {
            Region* region = GetRegionAtGrid(world, pos.first, pos.second);
            if (region) {
                RegionID id = region->GetID();
                f32 wx = region->GetX();
                f32 wy = region->GetY();
                auto& regions = world->GetRegions();
                
                regions[id] = std::make_unique<Region>(id, "RiverSource");
                regions[id]->SetPosition(wx, wy);
                regions[id]->SetIsSource(true);
                regions[id]->SetName(GetRandomName(def));
                regions[id]->Initialize();
                
                world->AddSourceRegion(id);
                created_sources.push_back(id);
            }
        }
    }
    
    std::cout << "Created " << created_sources.size() << " river sources" << std::endl;
    
    return created_sources;
}

void StandardWorldGenerator::Pass_ExpandRiverFromSource(
    World* world,
    RegionID source_id,
    const RegionDefinition& def) {
    (void)def;  // Parameter kept for consistency with other expansion functions
    
    Region* source_region = world->GetRegion(source_id);
    if (!source_region) {
        return;
    }
    
    f32 source_wx = source_region->GetX();
    f32 source_wy = source_region->GetY();
    u16 source_x = static_cast<u16>(source_wx / region_size_);
    u16 source_y = static_cast<u16>(source_wy / region_size_);
    
    // Find nearest coastal region
    std::pair<u16, u16> nearest_destination;
    bool found_destination = false;
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
                    nearest_destination = {cx, cy};
                    found_destination = true;
                }
            }
        }
    }
    
    // If no coastal regions found, find nearest border instead
    if (!found_destination) {
        std::cout << "No coastal regions found, finding nearest border for river source " << source_id << std::endl;
        
        // Find nearest border position
        std::vector<std::pair<u16, u16>> border_positions;
        
        // Top border
        for (u16 x = 0; x < grid_width_; ++x) {
            border_positions.push_back({x, 0});
        }
        // Bottom border
        for (u16 x = 0; x < grid_width_; ++x) {
            border_positions.push_back({x, grid_height_ - 1});
        }
        // Left border
        for (u16 y = 0; y < grid_height_; ++y) {
            border_positions.push_back({0, y});
        }
        // Right border
        for (u16 y = 0; y < grid_height_; ++y) {
            border_positions.push_back({grid_width_ - 1, y});
        }
        
        // Find nearest border position
        for (const auto& border_pos : border_positions) {
            f32 dist = std::sqrt(
                std::pow(static_cast<f32>(source_x) - static_cast<f32>(border_pos.first), 2.0f) +
                std::pow(static_cast<f32>(source_y) - static_cast<f32>(border_pos.second), 2.0f)
            );
            if (dist < min_dist) {
                min_dist = dist;
                nearest_destination = border_pos;
                found_destination = true;
            }
        }
    }
    
    if (!found_destination) {
        std::cout << "Warning: Could not find destination for river source " << source_id << std::endl;
        return;
    }
    
    // Create river path with merging logic
    std::vector<std::pair<u16, u16>> river_path = FindPathWithMerging(world, {source_x, source_y}, nearest_destination);
    
    // Place river regions along the path
    // Rivers can flow through most terrain types (except those that prevent overwrite)
    u32 river_count = 0;
    for (const auto& path_pos : river_path) {
        Region* region = GetRegionAtGrid(world, path_pos.first, path_pos.second);
        if (!region) {
            continue;
        }
        
        // Skip certain region types that rivers cannot flow through
        if (region->GetType() == "Water" || region->GetType() == "Coastal" || 
            region->GetType() == "Mountain" || region->GetType() == "RiverSource") {
            continue;
        }
        
        // Check if this region has prevent_overwrite set
        auto& config = Config::Configuration::GetInstance();
        const auto& region_defs = config.regions.region_definitions;
        auto existing_region_it = region_defs.find(region->GetType());
        if (existing_region_it != region_defs.end() && existing_region_it->second.prevent_overwrite) {
            continue;  // Skip regions that prevent overwrite
        }
        
        // Rivers can flow through any other terrain (including Desert, Forest, Plains, etc.)
        RegionID id = region->GetID();
        f32 rx_world = region->GetX();
        f32 ry_world = region->GetY();
        auto& regions = world->GetRegions();
        
        // Preserve source info if it exists
        bool was_source = region->IsSource();
        std::string region_name = region->GetName();
        RegionID parent_id = region->GetSourceParentID();
        
        regions[id] = std::make_unique<Region>(id, "River");
        regions[id]->SetPosition(rx_world, ry_world);
        regions[id]->SetSourceParentID(source_id);
        if (was_source) {
            regions[id]->SetIsSource(true);
            regions[id]->SetName(region_name);
        } else if (parent_id != INVALID_REGION_ID) {
            // Keep the original parent if it exists
            regions[id]->SetSourceParentID(parent_id);
        }
        regions[id]->Initialize();
        river_count++;
    }
    
    std::cout << "Created " << river_count << " river regions from source " << source_id << std::endl;
}

void StandardWorldGenerator::Pass_Settlements(
    World* world,
    const std::unordered_map<std::string, RegionDefinition>& region_definitions) {
    
    std::cout << "\n=== Pass: Settlements ===" << std::endl;
    
    auto urban_it = region_definitions.find("Urban");
    auto rural_it = region_definitions.find("Rural");
    
    if (urban_it == region_definitions.end() || rural_it == region_definitions.end()) {
        std::cout << "Pass_Settlements: Skipped - Urban or Rural types not in config" << std::endl;
        return;
    }
    
    const RegionDefinition& urban_def = urban_it->second;
    const RegionDefinition& rural_def = rural_it->second;
    
    std::vector<World::Settlement> settlements;
    
    // Minimum distance between settlements (in grid cells)
    const u16 min_settlement_distance = 12;
    
    // Helper function to check if a position is far enough from existing settlements
    auto IsFarEnoughFromSettlements = [&](u16 x, u16 y) -> bool {
        for (const auto& settlement : settlements) {
            f32 dist = std::sqrt(
                std::pow(static_cast<f32>(x) - static_cast<f32>(settlement.grid_x), 2.0f) +
                std::pow(static_cast<f32>(y) - static_cast<f32>(settlement.grid_y), 2.0f)
            );
            if (dist < static_cast<f32>(min_settlement_distance)) {
                return false;
            }
        }
        return true;
    };
    
    // Helper function to score a candidate (higher = better, prefers farther from existing settlements)
    auto ScoreCandidate = [&](u16 x, u16 y) -> f32 {
        if (settlements.empty()) {
            return 1.0f;  // First settlement gets neutral score
        }
        
        f32 min_dist = std::numeric_limits<f32>::max();
        for (const auto& settlement : settlements) {
            f32 dist = std::sqrt(
                std::pow(static_cast<f32>(x) - static_cast<f32>(settlement.grid_x), 2.0f) +
                std::pow(static_cast<f32>(y) - static_cast<f32>(settlement.grid_y), 2.0f)
            );
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
        // Score based on distance - farther is better
        return min_dist / static_cast<f32>(grid_width_ + grid_height_);
    };
    
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
            
            if (near_water && CanPlaceRegion(world, x, y, urban_def)) {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (!candidates.empty()) {
        // Filter candidates to only those far enough from existing settlements
        std::vector<std::pair<u16, u16>> valid_candidates;
        for (const auto& candidate : candidates) {
            if (IsFarEnoughFromSettlements(candidate.first, candidate.second)) {
                valid_candidates.push_back(candidate);
            }
        }
        
        // If no valid candidates, try with reduced distance requirement
        if (valid_candidates.empty()) {
            valid_candidates = candidates;
        }
        
        // Score candidates and pick from top candidates (prefer farther from existing)
        std::vector<std::pair<std::pair<u16, u16>, f32>> scored_candidates;
        for (const auto& candidate : valid_candidates) {
            f32 score = ScoreCandidate(candidate.first, candidate.second);
            scored_candidates.push_back({candidate, score});
        }
        
        // Sort by score (highest first)
        std::sort(scored_candidates.begin(), scored_candidates.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Pick from top 30% of candidates (or at least top 3, or all if less than 3)
        u32 top_count = std::max(1u, std::min(static_cast<u32>(scored_candidates.size()), 
                                               std::max(3u, static_cast<u32>(scored_candidates.size() * 0.3f))));
        u32 selected_idx = random_->RandomU32(0, top_count);
        auto pos = scored_candidates[selected_idx].first;
        
        Region* region = GetRegionAtGrid(world, pos.first, pos.second);
        if (region) {
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
            
            World::Settlement settlement;
            settlement.region_id = id;
            settlement.type = "City";
            settlement.grid_x = pos.first;
            settlement.grid_y = pos.second;
            settlements.push_back(settlement);
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
            
            if (near_mountain && CanPlaceRegion(world, x, y, urban_def)) {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (!candidates.empty()) {
        // Filter candidates to only those far enough from existing settlements
        std::vector<std::pair<u16, u16>> valid_candidates;
        for (const auto& candidate : candidates) {
            if (IsFarEnoughFromSettlements(candidate.first, candidate.second)) {
                valid_candidates.push_back(candidate);
            }
        }
        
        // If no valid candidates, try with reduced distance requirement
        if (valid_candidates.empty()) {
            valid_candidates = candidates;
        }
        
        // Score candidates and pick from top candidates
        std::vector<std::pair<std::pair<u16, u16>, f32>> scored_candidates;
        for (const auto& candidate : valid_candidates) {
            f32 score = ScoreCandidate(candidate.first, candidate.second);
            scored_candidates.push_back({candidate, score});
        }
        
        std::sort(scored_candidates.begin(), scored_candidates.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        u32 top_count = std::max(1u, std::min(static_cast<u32>(scored_candidates.size()), 
                                               std::max(3u, static_cast<u32>(scored_candidates.size() * 0.3f))));
        u32 selected_idx = random_->RandomU32(0, top_count);
        auto pos = scored_candidates[selected_idx].first;
        
        Region* region = GetRegionAtGrid(world, pos.first, pos.second);
        if (region) {
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
            
            World::Settlement settlement;
            settlement.region_id = id;
            settlement.type = "City";
            settlement.grid_x = pos.first;
            settlement.grid_y = pos.second;
            settlements.push_back(settlement);
        }
    }
    
    // Find settlement in plains
    candidates.clear();
    for (u16 y = 0; y < grid_height_; ++y) {
        for (u16 x = 0; x < grid_width_; ++x) {
            Region* region = GetRegionAtGrid(world, x, y);
            if (region && region->GetType() == "Plains" && CanPlaceRegion(world, x, y, rural_def)) {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (!candidates.empty()) {
        // Filter candidates to only those far enough from existing settlements
        std::vector<std::pair<u16, u16>> valid_candidates;
        for (const auto& candidate : candidates) {
            if (IsFarEnoughFromSettlements(candidate.first, candidate.second)) {
                valid_candidates.push_back(candidate);
            }
        }
        
        // If no valid candidates, try with reduced distance requirement
        if (valid_candidates.empty()) {
            valid_candidates = candidates;
        }
        
        // Score candidates and pick from top candidates
        std::vector<std::pair<std::pair<u16, u16>, f32>> scored_candidates;
        for (const auto& candidate : valid_candidates) {
            f32 score = ScoreCandidate(candidate.first, candidate.second);
            scored_candidates.push_back({candidate, score});
        }
        
        std::sort(scored_candidates.begin(), scored_candidates.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        u32 top_count = std::max(1u, std::min(static_cast<u32>(scored_candidates.size()), 
                                               std::max(3u, static_cast<u32>(scored_candidates.size() * 0.3f))));
        u32 selected_idx = random_->RandomU32(0, top_count);
        auto pos = scored_candidates[selected_idx].first;
        
        Region* region = GetRegionAtGrid(world, pos.first, pos.second);
        if (region) {
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
            
            World::Settlement settlement;
            settlement.region_id = id;
            settlement.type = "Village";
            settlement.grid_x = pos.first;
            settlement.grid_y = pos.second;
            settlements.push_back(settlement);
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
            
            if (near_forest && CanPlaceRegion(world, x, y, rural_def)) {
                candidates.push_back({x, y});
            }
        }
    }
    
    if (!candidates.empty()) {
        // Filter candidates to only those far enough from existing settlements
        std::vector<std::pair<u16, u16>> valid_candidates;
        for (const auto& candidate : candidates) {
            if (IsFarEnoughFromSettlements(candidate.first, candidate.second)) {
                valid_candidates.push_back(candidate);
            }
        }
        
        // If no valid candidates, try with reduced distance requirement
        if (valid_candidates.empty()) {
            valid_candidates = candidates;
        }
        
        // Score candidates and pick from top candidates
        std::vector<std::pair<std::pair<u16, u16>, f32>> scored_candidates;
        for (const auto& candidate : valid_candidates) {
            f32 score = ScoreCandidate(candidate.first, candidate.second);
            scored_candidates.push_back({candidate, score});
        }
        
        std::sort(scored_candidates.begin(), scored_candidates.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        u32 top_count = std::max(1u, std::min(static_cast<u32>(scored_candidates.size()), 
                                               std::max(3u, static_cast<u32>(scored_candidates.size() * 0.3f))));
        u32 selected_idx = random_->RandomU32(0, top_count);
        auto pos = scored_candidates[selected_idx].first;
        
        Region* region = GetRegionAtGrid(world, pos.first, pos.second);
        if (region) {
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
            
            World::Settlement settlement;
            settlement.region_id = id;
            settlement.type = "Village";
            settlement.grid_x = pos.first;
            settlement.grid_y = pos.second;
            settlements.push_back(settlement);
        }
    }
    
    // Place capital as central as possible to settlements
    if (!settlements.empty()) {
        std::vector<std::pair<u16, u16>> settlement_positions;
        for (const auto& settlement : settlements) {
            settlement_positions.push_back({settlement.grid_x, settlement.grid_y});
        }
        
        auto centroid = CalculateCentroid(settlement_positions);
        
        std::pair<u16, u16> best_pos = centroid;
        f32 best_dist = std::numeric_limits<f32>::max();
        
        for (i16 dy = -10; dy <= 10; ++dy) {
            for (i16 dx = -10; dx <= 10; ++dx) {
                i16 nx = static_cast<i16>(centroid.first) + dx;
                i16 ny = static_cast<i16>(centroid.second) + dy;
                
                if (nx >= 0 && nx < static_cast<i16>(grid_width_) &&
                    ny >= 0 && ny < static_cast<i16>(grid_height_)) {
                    u16 gx = static_cast<u16>(nx);
                    u16 gy = static_cast<u16>(ny);
                    
                    // Prefer positions that are far enough from existing settlements
                    if (!IsFarEnoughFromSettlements(gx, gy)) {
                        continue;
                    }
                    
                    Region* region = GetRegionAtGrid(world, gx, gy);
                    if (region && region->GetType() != "Water" && region->GetType() != "Mountain" && 
                        region->GetType() != "Coastal" && CanPlaceRegion(world, gx, gy, urban_def)) {
                        f32 dist = std::sqrt(
                            std::pow(static_cast<f32>(nx) - static_cast<f32>(centroid.first), 2.0f) +
                            std::pow(static_cast<f32>(ny) - static_cast<f32>(centroid.second), 2.0f)
                        );
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_pos = {gx, gy};
                        }
                    }
                }
            }
        }
        
        // If no position found with distance requirement, try without it
        if (best_pos.first == centroid.first && best_pos.second == centroid.second) {
            for (i16 dy = -10; dy <= 10; ++dy) {
                for (i16 dx = -10; dx <= 10; ++dx) {
                    i16 nx = static_cast<i16>(centroid.first) + dx;
                    i16 ny = static_cast<i16>(centroid.second) + dy;
                    
                    if (nx >= 0 && nx < static_cast<i16>(grid_width_) &&
                        ny >= 0 && ny < static_cast<i16>(grid_height_)) {
                        Region* region = GetRegionAtGrid(world, static_cast<u16>(nx), static_cast<u16>(ny));
                        if (region && region->GetType() != "Water" && region->GetType() != "Mountain" && 
                            region->GetType() != "Coastal" && CanPlaceRegion(world, static_cast<u16>(nx), static_cast<u16>(ny), urban_def)) {
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
        }
        
        Region* region = GetRegionAtGrid(world, best_pos.first, best_pos.second);
        if (region) {
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
            
            World::Settlement settlement;
            settlement.region_id = id;
            settlement.type = "Capital";
            settlement.grid_x = best_pos.first;
            settlement.grid_y = best_pos.second;
            settlements.push_back(settlement);
        }
    }
    
    // Expand Urban and Rural sources
    const auto& all_source_regions = world->GetSourceRegions();
    for (RegionID source_id : all_source_regions) {
        Region* source_region = world->GetRegion(source_id);
        if (source_region && (source_region->GetType() == "Urban" || source_region->GetType() == "Rural")) {
            auto it = region_definitions.find(source_region->GetType());
            if (it != region_definitions.end() && it->second.max_expansion_size > 0) {
                Pass_ExpandFromSource(world, source_id, it->second);
            }
        }
    }
    
    // Add all settlements to world
    for (const auto& settlement : settlements) {
        world->AddSettlement(settlement);
    }
    
    std::cout << "Placed " << settlements.size() << " settlements" << std::endl;
}

void StandardWorldGenerator::Pass_Roads(
    World* world,
    const std::unordered_map<std::string, RegionDefinition>& region_definitions) {
    
    std::cout << "\n=== Pass: Roads ===" << std::endl;
    
    auto road_it = region_definitions.find("Road");
    if (road_it == region_definitions.end()) {
        std::cout << "Pass_Roads: Skipped - Road type not in region definitions" << std::endl;
        return;
    }
    
    const auto& settlements = world->GetSettlements();
    if (settlements.size() < 2) {
        std::cout << "Pass_Roads: Skipped - Not enough settlements" << std::endl;
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
                        // Check if this region has prevent_overwrite set
                        auto& config = Config::Configuration::GetInstance();
                        const auto& region_defs = config.regions.region_definitions;
                        auto existing_region_it = region_defs.find(region->GetType());
                        if (existing_region_it != region_defs.end() && existing_region_it->second.prevent_overwrite) {
                            continue;  // Skip regions that prevent overwrite
                        }
                        
                        if (region->GetType() == "Plains" || region->GetType() == "Forest" || 
                            region->GetType() == "Desert" ||
                            region->GetType() == "Road") {
                            RegionID id = region->GetID();
                            f32 rx_world = region->GetX();
                            f32 ry_world = region->GetY();
                            auto& regions = world->GetRegions();
                            
                            bool was_source = region->IsSource();
                            std::string region_name = region->GetName();
                            RegionID parent_id = region->GetSourceParentID();
                            
                            regions[id] = std::make_unique<Region>(id, "Road");
                            regions[id]->SetPosition(rx_world, ry_world);
                            if (was_source) {
                                regions[id]->SetIsSource(true);
                                regions[id]->SetName(region_name);
                            } else if (parent_id != INVALID_REGION_ID) {
                                regions[id]->SetSourceParentID(parent_id);
                            }
                            regions[id]->Initialize();
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "Created " << world->GetRoads().size() << " roads" << std::endl;
}

std::string StandardWorldGenerator::GetRandomName(const RegionDefinition& def) {
    if (def.potential_names.empty()) {
        return def.type;
    }
    u32 idx = random_->RandomU32(0, static_cast<u32>(def.potential_names.size()));
    return def.potential_names[idx];
}

bool StandardWorldGenerator::CanPlaceRegion(World* world, u16 x, u16 y, const RegionDefinition& def) {
    Region* region = GetRegionAtGrid(world, x, y);
    if (!region) {
        return false;
    }

    // Check if the existing region has prevent_overwrite set
    auto& config = Config::Configuration::GetInstance();
    const auto& region_definitions = config.regions.region_definitions;
    auto existing_region_it = region_definitions.find(region->GetType());
    if (existing_region_it != region_definitions.end() && existing_region_it->second.prevent_overwrite) {
        return false;  // Cannot overwrite a region that has prevent_overwrite set
    }

    // Never allow any region to overwrite an existing Coastal region
    if (region->GetType() == "Coastal") {
        return false;
    }
    
    // Prevent non-coastal regions from being placed on borders that have coastal regions
    if (def.type != "Coastal" && IsOnRim(x, y)) {
        bool on_top = (y == 0);
        bool on_bottom = (y == grid_height_ - 1);
        bool on_left = (x == 0);
        bool on_right = (x == grid_width_ - 1);
        
        if ((on_top && coastal_borders_.find("top") != coastal_borders_.end()) ||
            (on_bottom && coastal_borders_.find("bottom") != coastal_borders_.end()) ||
            (on_left && coastal_borders_.find("left") != coastal_borders_.end()) ||
            (on_right && coastal_borders_.find("right") != coastal_borders_.end())) {
            return false;
        }
    }
    
    // Check if current type is compatible
    bool can_place_on = false;
    if (region->GetType() == "Plains" || region->GetType() == def.type) {
        can_place_on = true;
    } else {
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
        
        // Check if we've reached the destination
        if (current_x == end_x && current_y == end_y) {
            // Add destination to path if not already there
            if (path.empty() || path.back().first != end.first || path.back().second != end.second) {
                path.push_back({static_cast<u16>(end_x), static_cast<u16>(end_y)});
            }
            break;
        }
        
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
            // We're adjacent to destination, move to it and add it
            current_x = end_x;
            current_y = end_y;
            u16 rx = static_cast<u16>(current_x);
            u16 ry = static_cast<u16>(current_y);
            if (rx != start.first || ry != start.second) {
                path.push_back({rx, ry});
            }
            break;
        }
        
        i16 move_x = 0;
        i16 move_y = 0;
        
        if (std::abs(dx) > std::abs(dy)) {
            move_x = (dx > 0) ? 1 : -1;
        } else {
            move_y = (dy > 0) ? 1 : -1;
        }
        
        if (random_->RandomFloat(0.0f, 1.0f) < 0.15f) {
            if (move_x != 0) {
                move_y = random_->RandomBool(0.5f) ? 1 : -1;
            } else {
                move_x = random_->RandomBool(0.5f) ? 1 : -1;
            }
        }
        
        current_x += move_x;
        current_y += move_y;
        
        if (current_x < 0) current_x = 0;
        if (current_x >= static_cast<i16>(grid_width_)) current_x = static_cast<i16>(grid_width_) - 1;
        if (current_y < 0) current_y = 0;
        if (current_y >= static_cast<i16>(grid_height_)) current_y = static_cast<i16>(grid_height_) - 1;
        
        u16 rx = static_cast<u16>(current_x);
        u16 ry = static_cast<u16>(current_y);
        
        u32 pos_key = static_cast<u32>(ry) * static_cast<u32>(grid_width_) + static_cast<u32>(rx);
        if (visited_positions.find(pos_key) != visited_positions.end()) {
            break;
        }
        
        visited_positions.insert(pos_key);
        
        if (rx != start.first || ry != start.second) {
            path.push_back({rx, ry});
        }
    }
    
    // Ensure the destination is included in the path if we're close but didn't reach it
    if (!path.empty()) {
        auto& last_pos = path.back();
        i16 dx = end_x - static_cast<i16>(last_pos.first);
        i16 dy = end_y - static_cast<i16>(last_pos.second);
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1 && (dx != 0 || dy != 0)) {
            // Add the destination if we're adjacent but didn't reach it
            path.push_back({static_cast<u16>(end_x), static_cast<u16>(end_y)});
        }
    } else if (start.first != end.first || start.second != end.second) {
        // If path is empty and start != end, add destination
        path.push_back({static_cast<u16>(end_x), static_cast<u16>(end_y)});
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
    const u16 merge_search_radius = 5;
    
    while (step_count < max_path_length) {
        step_count++;
        i16 dx = end_x - current_x;
        i16 dy = end_y - current_y;
        
        // Check if we've reached the destination
        if (current_x == end_x && current_y == end_y) {
            // Add destination to path if not already there
            if (path.empty() || path.back().first != end.first || path.back().second != end.second) {
                path.push_back({static_cast<u16>(end_x), static_cast<u16>(end_y)});
            }
            break;
        }
        
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
            // We're adjacent to destination, move to it and add it
            current_x = end_x;
            current_y = end_y;
            u16 rx = static_cast<u16>(current_x);
            u16 ry = static_cast<u16>(current_y);
            if (rx != start.first || ry != start.second) {
                path.push_back({rx, ry});
            }
            break;
        }
        
        i16 move_x = 0;
        i16 move_y = 0;
        
        if (std::abs(dx) > std::abs(dy)) {
            move_x = (dx > 0) ? 1 : -1;
        } else {
            move_y = (dy > 0) ? 1 : -1;
        }
        
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
                    u32 check_pos_key = static_cast<u32>(check_y) * static_cast<u32>(grid_width_) + static_cast<u32>(check_x);
                    if (visited_positions.find(check_pos_key) != visited_positions.end()) {
                        continue;
                    }
                    
                    f32 dist = std::sqrt(
                        std::pow(static_cast<f32>(check_x - current_x), 2.0f) +
                        std::pow(static_cast<f32>(check_y - current_y), 2.0f)
                    );
                    
                    if (dist < nearest_river_dist && dist > 1.0f) {
                        nearest_river_dist = dist;
                        nearest_river = {static_cast<u16>(check_x), static_cast<u16>(check_y)};
                        found_river = true;
                    }
                }
            }
        }
        
        i16 final_move_x = move_x;
        i16 final_move_y = move_y;
        
        if (found_river && nearest_river_dist < static_cast<f32>(merge_search_radius)) {
            i16 merge_dx = static_cast<i16>(nearest_river.first) - current_x;
            i16 merge_dy = static_cast<i16>(nearest_river.second) - current_y;
            
            if (std::abs(merge_dx) > std::abs(merge_dy)) {
                merge_dx = (merge_dx > 0) ? 1 : -1;
                merge_dy = 0;
            } else {
                merge_dx = 0;
                merge_dy = (merge_dy > 0) ? 1 : -1;
            }
            
            if (random_->RandomFloat(0.0f, 1.0f) < 0.40f) {
                final_move_x = merge_dx;
                final_move_y = merge_dy;
            } else {
                if (merge_dx != 0 && move_x == 0) {
                    final_move_x = merge_dx;
                }
                if (merge_dy != 0 && move_y == 0) {
                    final_move_y = merge_dy;
                }
            }
        } else {
            if (random_->RandomFloat(0.0f, 1.0f) < 0.15f) {
                if (move_x != 0) {
                    final_move_y = random_->RandomBool(0.5f) ? 1 : -1;
                } else {
                    final_move_x = random_->RandomBool(0.5f) ? 1 : -1;
                }
            }
        }
        
        current_x += final_move_x;
        current_y += final_move_y;
        
        if (current_x < 0) current_x = 0;
        if (current_x >= static_cast<i16>(grid_width_)) current_x = static_cast<i16>(grid_width_) - 1;
        if (current_y < 0) current_y = 0;
        if (current_y >= static_cast<i16>(grid_height_)) current_y = static_cast<i16>(grid_height_) - 1;
        
        u16 rx = static_cast<u16>(current_x);
        u16 ry = static_cast<u16>(current_y);
        
        u32 pos_key = static_cast<u32>(ry) * static_cast<u32>(grid_width_) + static_cast<u32>(rx);
        if (visited_positions.find(pos_key) != visited_positions.end()) {
            break;
        }
        
        visited_positions.insert(pos_key);
        
        if (rx != start.first || ry != start.second) {
            path.push_back({rx, ry});
        }
    }
    
    // Ensure the destination is included in the path if we're close but didn't reach it
    if (!path.empty()) {
        auto& last_pos = path.back();
        i16 dx = end_x - static_cast<i16>(last_pos.first);
        i16 dy = end_y - static_cast<i16>(last_pos.second);
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1 && (dx != 0 || dy != 0)) {
            // Add the destination if we're adjacent but didn't reach it
            path.push_back({static_cast<u16>(end_x), static_cast<u16>(end_y)});
        }
    } else if (start.first != end.first || start.second != end.second) {
        // If path is empty and start != end, add destination
        path.push_back({static_cast<u16>(end_x), static_cast<u16>(end_y)});
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
    struct Node {
        u16 x, y;
        u32 g_cost;
        u32 h_cost;
        u32 f_cost() const { return g_cost + h_cost; }
        std::pair<u16, u16> parent;
        
        bool operator<(const Node& other) const {
            return f_cost() > other.f_cost();
        }
    };
    
    auto GetTerrainCost = [](const std::string& region_type) -> u32 {
        if (region_type == "Plains" || region_type == "Urban" || region_type == "Rural" || region_type == "Road") {
            return 1;
        } else if (region_type == "Forest" || region_type == "Mountain") {
            return 10;
        } else if (region_type == "Water" || region_type == "Coastal") {
            return 1000;
        } else {
            return 5;
        }
    };
    
    auto Heuristic = [](u16 x1, u16 y1, u16 x2, u16 y2) -> u32 {
        return static_cast<u32>(std::abs(static_cast<i32>(x1) - static_cast<i32>(x2)) + 
                                std::abs(static_cast<i32>(y1) - static_cast<i32>(y2)));
    };
    
    std::priority_queue<Node> open_set;
    std::unordered_map<u32, Node> all_nodes;
    std::unordered_set<u32> closed_set;
    
    Node start_node;
    start_node.x = start.first;
    start_node.y = start.second;
    start_node.g_cost = 0;
    start_node.h_cost = Heuristic(start.first, start.second, end.first, end.second);
    start_node.parent = {0xFFFF, 0xFFFF};
    
    u32 start_key = static_cast<u32>(start.second) * static_cast<u32>(grid_width_) + static_cast<u32>(start.first);
    all_nodes[start_key] = start_node;
    open_set.push(start_node);
    
    const i16 dx[] = {0, 1, 0, -1};
    const i16 dy[] = {-1, 0, 1, 0};
    
    while (!open_set.empty()) {
        Node current = open_set.top();
        open_set.pop();
        
        u32 current_key = static_cast<u32>(current.y) * static_cast<u32>(grid_width_) + static_cast<u32>(current.x);
        
        if (closed_set.find(current_key) != closed_set.end()) {
            continue;
        }
        
        closed_set.insert(current_key);
        
        if (current.x == end.first && current.y == end.second) {
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
                continue;
            }
            
            Region* region = GetRegionAtGrid(world, gx, gy);
            if (!region) {
                continue;
            }
            
            std::string region_type = region->GetType();
            if (region_type.empty()) {
                continue;
            }
            
            u32 terrain_cost = GetTerrainCost(region_type);
            u32 new_g_cost = current.g_cost + terrain_cost;
            
            auto it = all_nodes.find(neighbor_key);
            if (it != all_nodes.end()) {
                if (new_g_cost < it->second.g_cost) {
                    it->second.g_cost = new_g_cost;
                    it->second.parent = {current.x, current.y};
                    open_set.push(it->second);
                }
            } else {
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
    
    return std::vector<std::pair<u16, u16>>();
}

} // namespace Simulation