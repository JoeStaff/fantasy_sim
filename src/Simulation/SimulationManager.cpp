#include "Simulation/SimulationManager.h"
#include "Simulation/LODSystem.h"
#include "Simulation/Region.h"
#include "Core/Config.h"
#include "Utils/Random.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include <utility>
#include <limits>

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
    // Find region by ID (regions are stored in order, but ID might not match index)
    for (auto& region : regions_) {
        if (region && region->GetID() == region_id) {
            return region.get();
        }
    }
    return nullptr;
}

const Region* SimulationManager::GetRegion(RegionID region_id) const {
    // Find region by ID (regions are stored in order, but ID might not match index)
    for (const auto& region : regions_) {
        if (region && region->GetID() == region_id) {
            return region.get();
        }
    }
    return nullptr;
}

const std::vector<std::unique_ptr<Region>>& SimulationManager::GetRegions() const {
    return regions_;
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
    // Get config
    auto& config = Config::Configuration::GetInstance();
    const auto& region_types = config.regions.types;
    
    std::cout << "InitializeRegionGrid: region_types.size() = " << region_types.size() << std::endl;
    
    if (region_types.empty()) {
        std::cout << "InitializeRegionGrid: ERROR - No region types in config!" << std::endl;
        return;
    }
    
    // Calculate total regions needed for the grid
    u32 total_regions = static_cast<u32>(grid_width) * static_cast<u32>(grid_height);
    
    // Clear existing regions
    regions_.clear();
    regions_.reserve(total_regions);
    
    // Get random instance
    auto& random = Utils::Random::GetInstance();
    
    // Initialize all regions as "Plains" (default/placeholder)
    for (u32 i = 0; i < total_regions; ++i) {
        u16 x = static_cast<u16>(i % grid_width);
        u16 y = static_cast<u16>(i / grid_width);
        f32 world_x = static_cast<f32>(x) * region_size;
        f32 world_y = static_cast<f32>(y) * region_size;
        
        auto region = std::make_unique<Region>(i, "Plains");
        region->SetPosition(world_x, world_y);
        region->Initialize();
        regions_.push_back(std::move(region));
    }
    
    // Helper function to get region at grid position
    auto GetRegionAtGrid = [&](u16 gx, u16 gy) -> Region* {
        if (gx >= grid_width || gy >= grid_height) {
            return nullptr;
        }
        u32 idx = static_cast<u32>(gy) * static_cast<u32>(grid_width) + static_cast<u32>(gx);
        if (idx < regions_.size()) {
            return regions_[idx].get();
        }
        return nullptr;
    };
    
    // Helper to check if position is on the rim (outer edge)
    auto IsOnRim = [&](u16 x, u16 y) -> bool {
        return (x == 0 || x == grid_width - 1 || y == 0 || y == grid_height - 1);
    };
    
    // Helper to check distance from rim
    auto DistanceFromRim = [&](u16 x, u16 y) -> u16 {
        u16 dist_x = std::min(x, static_cast<u16>(grid_width - 1 - x));
        u16 dist_y = std::min(y, static_cast<u16>(grid_height - 1 - y));
        return std::min(dist_x, dist_y);
    };
    
    // Helper to check if in northern or southern hemisphere
    auto IsInNorthernHemisphere = [&](u16 y) -> bool {
        return y < grid_height / 2;
    };
    
    std::cout << "InitializeRegionGrid: Starting multi-pass generation" << std::endl;
    
    // PASS 1: Place Coastal regions on the ENTIRE rim (always coastal)
    std::vector<std::pair<u16, u16>> coastal_positions;
    for (u16 y = 0; y < grid_height; ++y) {
        for (u16 x = 0; x < grid_width; ++x) {
            if (IsOnRim(x, y)) {
                coastal_positions.push_back({x, y});
                Region* region = GetRegionAtGrid(x, y);
                if (region) {
                    RegionID id = region->GetID();
                    f32 wx = region->GetX();
                    f32 wy = region->GetY();
                    regions_[id] = std::make_unique<Region>(id, "Coastal");
                    regions_[id]->SetPosition(wx, wy);
                    regions_[id]->Initialize();
                }
            }
        }
    }
    std::cout << "Pass 1: Placed " << coastal_positions.size() << " coastal regions (entire rim)" << std::endl;
    
    // PASS 2: Place Water bodies (lakes) in interior, then create Rivers connecting to Coastal
    // Check if Water type exists
    bool water_type_exists = false;
    bool river_type_exists = false;
    for (const auto& type : region_types) {
        if (type == "Water") {
            water_type_exists = true;
        }
        if (type == "River") {
            river_type_exists = true;
        }
    }
    
    std::vector<std::pair<u16, u16>> water_positions;
    if (water_type_exists) {
        // Reduced water count (1-2% water)
        u32 water_count = static_cast<u32>(total_regions * random.RandomFloat(0.01f, 0.02f));
        u32 attempts = 0;
        while (water_positions.size() < water_count && attempts < water_count * 15) {
            u16 x = random.RandomU32(5, grid_width - 5);
            u16 y = random.RandomU32(5, grid_height - 5);
            // Ensure not on rim and not already water
            Region* region = GetRegionAtGrid(x, y);
            if (!IsOnRim(x, y) && region && region->GetType() == "Plains") {
                water_positions.push_back({x, y});
                RegionID id = region->GetID();
                f32 wx = region->GetX();
                f32 wy = region->GetY();
                regions_[id] = std::make_unique<Region>(id, "Water");
                regions_[id]->SetPosition(wx, wy);
                regions_[id]->Initialize();
            }
            attempts++;
        }
    }
    std::cout << "Pass 2: Placed " << water_positions.size() << " water bodies" << std::endl;
    
    // PASS 3: Place Mountains in clumps (3-5 mountain ranges, each 15-30 cells)
    u32 mountain_range_count = random.RandomU32(3, 6);
    for (u32 range = 0; range < mountain_range_count; ++range) {
        // Pick a center point (avoid rim and water)
        u16 center_x = random.RandomU32(10, grid_width - 10);
        u16 center_y = random.RandomU32(10, grid_height - 10);
        
        u32 mountain_size = random.RandomU32(15, 31);
        u32 placed = 0;
        
        // Place mountains in a clump around center
        std::vector<std::pair<u16, u16>> candidates;
        for (i16 dy = -8; dy <= 8 && placed < mountain_size; ++dy) {
            for (i16 dx = -8; dx <= 8 && placed < mountain_size; ++dx) {
                i16 nx = static_cast<i16>(center_x) + dx;
                i16 ny = static_cast<i16>(center_y) + dy;
                
                if (nx >= 0 && nx < static_cast<i16>(grid_width) &&
                    ny >= 0 && ny < static_cast<i16>(grid_height)) {
                    u16 gx = static_cast<u16>(nx);
                    u16 gy = static_cast<u16>(ny);
                    
                    Region* region = GetRegionAtGrid(gx, gy);
                    if (region && region->GetType() == "Plains") {
                        f32 dist = std::sqrt(static_cast<f32>(dx*dx + dy*dy));
                        f32 prob = 1.0f - (dist / 8.0f);  // Higher probability closer to center
                        if (random.RandomFloat(0.0f, 1.0f) < prob) {
                            RegionID id = region->GetID();
                            f32 mx = region->GetX();
                            f32 my = region->GetY();
                            regions_[id] = std::make_unique<Region>(id, "Mountain");
                            regions_[id]->SetPosition(mx, my);
                            regions_[id]->Initialize();
                            placed++;
                        }
                    }
                }
            }
        }
    }
    std::cout << "Pass 3: Placed mountain ranges" << std::endl;
    
    // PASS 3b: Generate RiverSource regions near mountains, then create river paths
    // Check if RiverSource type exists
    bool river_source_type_exists = false;
    for (const auto& type : region_types) {
        if (type == "RiverSource") {
            river_source_type_exists = true;
            break;
        }
    }
    
    std::cout << "Pass 3b: river_type_exists=" << river_type_exists 
              << ", river_source_type_exists=" << river_source_type_exists 
              << ", coastal_positions.size()=" << coastal_positions.size() << std::endl;
    
    u32 river_source_count = 0;
    u32 river_count = 0;
    
    if (river_type_exists && river_source_type_exists && !coastal_positions.empty()) {
        // Step 1: Place RiverSource regions near mountains (preferring regions adjacent to mountains)
        std::vector<std::pair<u16, u16>> river_source_positions;
        
        // Count mountain regions for debugging
        u32 mountain_count = 0;
        for (u16 y = 0; y < grid_height; ++y) {
            for (u16 x = 0; x < grid_width; ++x) {
                Region* region = GetRegionAtGrid(x, y);
                if (region && region->GetType() == "Mountain") {
                    mountain_count++;
                }
            }
        }
        std::cout << "Pass 3b: Found " << mountain_count << " mountain regions" << std::endl;
        
        // Find all regions adjacent to mountains
        u32 regions_adjacent_to_mountains = 0;
        for (u16 y = 0; y < grid_height; ++y) {
            for (u16 x = 0; x < grid_width; ++x) {
                Region* region = GetRegionAtGrid(x, y);
                if (!region || region->GetType() == "Mountain" || region->GetType() == "Water" || 
                    region->GetType() == "Coastal" || region->GetType() == "River" || region->GetType() == "RiverSource") {
                    continue;
                }
                
                // Check if this region is adjacent to a mountain
                Region* neighbors[4] = {
                    GetRegionAtGrid(x, y - 1),
                    GetRegionAtGrid(x, y + 1),
                    GetRegionAtGrid(x - 1, y),
                    GetRegionAtGrid(x + 1, y)
                };
                
                bool adjacent_to_mountain = false;
                for (Region* neighbor : neighbors) {
                    if (neighbor && neighbor->GetType() == "Mountain") {
                        adjacent_to_mountain = true;
                        break;
                    }
                }
                
                if (adjacent_to_mountain) {
                    regions_adjacent_to_mountains++;
                }
                
                // 10% chance to place RiverSource if adjacent to mountain (reduced significantly)
                if (adjacent_to_mountain && random.RandomFloat(0.0f, 1.0f) < 0.10f) {
                    // Check if already a river source
                    bool already_source = false;
                    for (const auto& source : river_source_positions) {
                        if (source.first == x && source.second == y) {
                            already_source = true;
                            break;
                        }
                    }
                    
                    if (!already_source) {
                        river_source_positions.push_back({x, y});
                        // Place the RiverSource region
                        RegionID id = region->GetID();
                        f32 wx = region->GetX();
                        f32 wy = region->GetY();
                        regions_[id] = std::make_unique<Region>(id, "RiverSource");
                        regions_[id]->SetPosition(wx, wy);
                        regions_[id]->Initialize();
                        river_source_count++;
                    }
                }
            }
        }
        
        std::cout << "Pass 3b: Found " << regions_adjacent_to_mountains << " regions adjacent to mountains" << std::endl;
        std::cout << "Pass 3b: Placed " << river_source_count << " RiverSource regions near mountains" << std::endl;
        
        if (river_source_positions.empty()) {
            std::cout << "Pass 3b: WARNING - No RiverSource positions found! Skipping river generation." << std::endl;
        }
        
        // Step 2: For each RiverSource, create a curvy path to nearest coastal region only
        u32 paths_created = 0;
        for (const auto& source_pos : river_source_positions) {
            // Find nearest coastal region only
            std::pair<u16, u16> nearest_target;
            bool found_target = false;
            f32 min_dist = std::numeric_limits<f32>::max();
            
            // Check coastal regions only
            for (const auto& coastal : coastal_positions) {
                f32 dist = std::sqrt(
                    std::pow(static_cast<f32>(source_pos.first) - static_cast<f32>(coastal.first), 2) +
                    std::pow(static_cast<f32>(source_pos.second) - static_cast<f32>(coastal.second), 2)
                );
                if (dist < min_dist) {
                    min_dist = dist;
                    nearest_target = coastal;
                    found_target = true;
                }
            }
            
            if (!found_target) {
                std::cout << "Pass 3b: WARNING - No coastal target found for RiverSource at (" 
                          << source_pos.first << ", " << source_pos.second << ")" << std::endl;
                continue;  // Skip if no target found
            }
            
            std::cout << "Pass 3b: Creating river path from (" << source_pos.first << ", " << source_pos.second 
                      << ") to (" << nearest_target.first << ", " << nearest_target.second << ")" << std::endl;
            
            // Create curvy river path that curves toward nearby rivers to combine
            i16 start_x = static_cast<i16>(source_pos.first);
            i16 start_y = static_cast<i16>(source_pos.second);
            i16 end_x = static_cast<i16>(nearest_target.first);
            i16 end_y = static_cast<i16>(nearest_target.second);
            
            i16 current_x = start_x;
            i16 current_y = start_y;
            std::vector<std::pair<u16, u16>> river_path;
            std::unordered_set<u32> visited_positions;  // Track visited positions to prevent loops
            // Don't include source in path (it's already a RiverSource)
            
            u32 max_path_length = static_cast<u32>(grid_width + grid_height) * 2;  // Safety limit
            u32 step_count = 0;
            
            while (step_count < max_path_length) {
                step_count++;
                i16 dx = end_x - current_x;
                i16 dy = end_y - current_y;
                
                // Check if we've reached the target
                if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
                    break;
                }
                
                // Calculate main direction (towards target) - deterministic
                i16 move_x = 0;
                i16 move_y = 0;
                
                if (std::abs(dx) > std::abs(dy)) {
                    move_x = (dx > 0) ? 1 : -1;
                } else {
                    move_y = (dy > 0) ? 1 : -1;
                }
                
                // Check for nearby rivers to curve toward (within 5 cells)
                i16 merge_x = 0;
                i16 merge_y = 0;
                f32 nearest_river_dist = 10.0f;  // Search radius
                
                for (i16 check_y = current_y - 5; check_y <= current_y + 5; ++check_y) {
                    for (i16 check_x = current_x - 5; check_x <= current_x + 5; ++check_x) {
                        if (check_x < 0 || check_x >= static_cast<i16>(grid_width) ||
                            check_y < 0 || check_y >= static_cast<i16>(grid_height)) {
                            continue;
                        }
                        
                        Region* check_region = GetRegionAtGrid(static_cast<u16>(check_x), static_cast<u16>(check_y));
                        // Only check for already-placed rivers (not RiverSource, as those might be generating)
                        if (check_region && check_region->GetType() == "River") {
                            // Don't count regions already in our path
                            u32 check_pos_key = static_cast<u32>(check_y) * static_cast<u32>(grid_width) + static_cast<u32>(check_x);
                            if (visited_positions.find(check_pos_key) != visited_positions.end()) {
                                continue;
                            }
                            
                            f32 dist = std::sqrt(
                                std::pow(static_cast<f32>(check_x - current_x), 2) +
                                std::pow(static_cast<f32>(check_y - current_y), 2)
                            );
                            
                            if (dist < nearest_river_dist && dist > 1.0f) {  // Not too close, not too far
                                nearest_river_dist = dist;
                                merge_x = check_x - current_x;
                                merge_y = check_y - current_y;
                            }
                        }
                    }
                }
                
                // Weight movement: 60% toward target, 40% toward nearby river (if found)
                i16 final_move_x = move_x;
                i16 final_move_y = move_y;
                
                if (nearest_river_dist < 10.0f && (merge_x != 0 || merge_y != 0)) {
                    // Normalize merge direction
                    f32 merge_len = std::sqrt(static_cast<f32>(merge_x * merge_x + merge_y * merge_y));
                    if (merge_len > 0.0f) {
                        i16 merge_dir_x = (merge_x > 0) ? 1 : ((merge_x < 0) ? -1 : 0);
                        i16 merge_dir_y = (merge_y > 0) ? 1 : ((merge_y < 0) ? -1 : 0);
                        
                        // 40% chance to move toward nearby river instead of directly toward target
                        if (random.RandomFloat(0.0f, 1.0f) < 0.40f) {
                            final_move_x = merge_dir_x;
                            final_move_y = merge_dir_y;
                        } else {
                            // Still move toward target, but add slight curve toward river
                            if (merge_dir_x != 0 && move_x == 0) {
                                final_move_x = merge_dir_x;
                            }
                            if (merge_dir_y != 0 && move_y == 0) {
                                final_move_y = merge_dir_y;
                            }
                        }
                    }
                } else {
                    // No nearby river found, use normal curvyness (15% chance to deviate)
                    if (random.RandomFloat(0.0f, 1.0f) < 0.15f) {
                        // Perpendicular offset for slight curvyness
                        if (move_x != 0) {
                            final_move_y = random.RandomBool(0.5f) ? 1 : -1;
                        } else {
                            final_move_x = random.RandomBool(0.5f) ? 1 : -1;
                        }
                    }
                }
                
                current_x += final_move_x;
                current_y += final_move_y;
                
                // Clamp to bounds
                if (current_x < 0) current_x = 0;
                if (current_x >= static_cast<i16>(grid_width)) current_x = static_cast<i16>(grid_width) - 1;
                if (current_y < 0) current_y = 0;
                if (current_y >= static_cast<i16>(grid_height)) current_y = static_cast<i16>(grid_height) - 1;
                
                u16 rx = static_cast<u16>(current_x);
                u16 ry = static_cast<u16>(current_y);
                
                // Skip if this is the source position
                if (rx == source_pos.first && ry == source_pos.second) {
                    // If we're stuck at source, force movement toward target
                    if (final_move_x == 0 && final_move_y == 0) {
                        final_move_x = move_x;
                        final_move_y = move_y;
                        current_x += final_move_x;
                        current_y += final_move_y;
                        // Re-clamp
                        if (current_x < 0) current_x = 0;
                        if (current_x >= static_cast<i16>(grid_width)) current_x = static_cast<i16>(grid_width) - 1;
                        if (current_y < 0) current_y = 0;
                        if (current_y >= static_cast<i16>(grid_height)) current_y = static_cast<i16>(grid_height) - 1;
                        rx = static_cast<u16>(current_x);
                        ry = static_cast<u16>(current_y);
                    } else {
                        continue;
                    }
                }
                
                // Check if we've visited this position before (loop detection)
                u32 pos_key = static_cast<u32>(ry) * static_cast<u32>(grid_width) + static_cast<u32>(rx);
                if (visited_positions.find(pos_key) != visited_positions.end()) {
                    // We're in a loop - force movement toward target only
                    final_move_x = move_x;
                    final_move_y = move_y;
                    current_x += final_move_x;
                    current_y += final_move_y;
                    // Re-clamp
                    if (current_x < 0) current_x = 0;
                    if (current_x >= static_cast<i16>(grid_width)) current_x = static_cast<i16>(grid_width) - 1;
                    if (current_y < 0) current_y = 0;
                    if (current_y >= static_cast<i16>(grid_height)) current_y = static_cast<i16>(grid_height) - 1;
                    rx = static_cast<u16>(current_x);
                    ry = static_cast<u16>(current_y);
                    pos_key = static_cast<u32>(ry) * static_cast<u32>(grid_width) + static_cast<u32>(rx);
                    // If still in loop after forced move, break
                    if (visited_positions.find(pos_key) != visited_positions.end()) {
                        break;
                    }
                }
                
                // Skip if this is the target (coastal)
                Region* target_region = GetRegionAtGrid(rx, ry);
                if (target_region && target_region->GetType() == "Coastal") {
                    break;
                }
                
                // Add to path and mark as visited
                river_path.push_back({rx, ry});
                visited_positions.insert(pos_key);
            }
            
            // Place river regions along the path
            u32 path_river_count = 0;
            for (const auto& path_pos : river_path) {
                Region* region = GetRegionAtGrid(path_pos.first, path_pos.second);
                if (region && region->GetType() != "Water" && region->GetType() != "Coastal" && 
                    region->GetType() != "Mountain" && region->GetType() != "RiverSource") {
                    // Use River type
                    RegionID id = region->GetID();
                    f32 rx_world = region->GetX();
                    f32 ry_world = region->GetY();
                    regions_[id] = std::make_unique<Region>(id, "River");
                    regions_[id]->SetPosition(rx_world, ry_world);
                    regions_[id]->Initialize();
                    river_count++;
                    path_river_count++;
                }
            }
            std::cout << "Pass 3b: Created " << path_river_count << " river cells for this path (total path length: " 
                      << river_path.size() << ")" << std::endl;
            paths_created++;
        }
        std::cout << "Pass 3b: Created " << paths_created << " river paths" << std::endl;
    } else {
        std::cout << "Pass 3b: SKIPPED - Conditions not met: river_type_exists=" << river_type_exists 
                  << ", river_source_type_exists=" << river_source_type_exists 
                  << ", coastal_positions.empty()=" << coastal_positions.empty() << std::endl;
    }
    std::cout << "Pass 3b: Created " << river_count << " river cells from " << river_source_count << " RiverSource regions" << std::endl;
    
    // PASS 4: Place inhabited regions (rare, clumped, varied size and shape)
    // First, place 1-2 capital regions (Urban clumps with varied sizes)
    u32 capital_count = random.RandomU32(1, 3);
    u32 total_urban_placed = 0;
    for (u32 cap = 0; cap < capital_count; ++cap) {
        u16 cap_x = random.RandomU32(20, grid_width - 20);
        u16 cap_y = random.RandomU32(20, grid_height - 20);
        
        // Varied capital size (8-20 cells, irregular shape)
        u32 capital_size = random.RandomU32(8, 21);
        u32 placed = 0;
        
        // Use a clumping algorithm with irregular shape
        std::vector<std::pair<u16, u16>> placed_cells;
        placed_cells.push_back({cap_x, cap_y});  // Start with center
        
        while (placed < capital_size && !placed_cells.empty()) {
            // Pick a random placed cell to grow from
            u32 seed_idx = random.RandomU32(0, static_cast<u32>(placed_cells.size()));
            u16 seed_x = placed_cells[seed_idx].first;
            u16 seed_y = placed_cells[seed_idx].second;
            
            // Try to place adjacent cells (with some randomness for irregular shape)
            for (i16 dy = -1; dy <= 1 && placed < capital_size; ++dy) {
                for (i16 dx = -1; dx <= 1 && placed < capital_size; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    
                    i16 nx = static_cast<i16>(seed_x) + dx;
                    i16 ny = static_cast<i16>(seed_y) + dy;
                    
                    if (nx >= 0 && nx < static_cast<i16>(grid_width) &&
                        ny >= 0 && ny < static_cast<i16>(grid_height)) {
                        
                        // Check if already placed
                        bool already_placed = false;
                        for (const auto& cell : placed_cells) {
                            if (cell.first == static_cast<u16>(nx) && cell.second == static_cast<u16>(ny)) {
                                already_placed = true;
                                break;
                            }
                        }
                        
                        if (!already_placed) {
                            Region* region = GetRegionAtGrid(static_cast<u16>(nx), static_cast<u16>(ny));
                            if (region && (region->GetType() == "Plains" || region->GetType() == "Forest")) {
                                // 70% chance to place (creates irregular shape)
                                if (random.RandomFloat(0.0f, 1.0f) < 0.7f) {
                                    RegionID id = region->GetID();
                                    f32 ux = region->GetX();
                                    f32 uy = region->GetY();
                                    regions_[id] = std::make_unique<Region>(id, "Urban");
                                    regions_[id]->SetPosition(ux, uy);
                                    regions_[id]->Initialize();
                                    placed_cells.push_back({static_cast<u16>(nx), static_cast<u16>(ny)});
                                    placed++;
                                    total_urban_placed++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Place Rural regions in clumps (very rare, 0.3-0.7% of map, but clumped)
    u32 rural_clump_count = random.RandomU32(2, 6);  // 2-5 rural clumps
    u32 total_rural_placed = 0;
    for (u32 clump = 0; clump < rural_clump_count; ++clump) {
        u16 rural_x = random.RandomU32(10, grid_width - 10);
        u16 rural_y = random.RandomU32(10, grid_height - 10);
        
        // Varied rural clump size (3-8 cells)
        u32 rural_size = random.RandomU32(3, 9);
        u32 placed = 0;
        
        std::vector<std::pair<u16, u16>> rural_cells;
        rural_cells.push_back({rural_x, rural_y});
        
        while (placed < rural_size && !rural_cells.empty()) {
            u32 seed_idx = random.RandomU32(0, static_cast<u32>(rural_cells.size()));
            u16 seed_x = rural_cells[seed_idx].first;
            u16 seed_y = rural_cells[seed_idx].second;
            
            for (i16 dy = -1; dy <= 1 && placed < rural_size; ++dy) {
                for (i16 dx = -1; dx <= 1 && placed < rural_size; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    
                    i16 nx = static_cast<i16>(seed_x) + dx;
                    i16 ny = static_cast<i16>(seed_y) + dy;
                    
                    if (nx >= 0 && nx < static_cast<i16>(grid_width) &&
                        ny >= 0 && ny < static_cast<i16>(grid_height)) {
                        
                        bool already_placed = false;
                        for (const auto& cell : rural_cells) {
                            if (cell.first == static_cast<u16>(nx) && cell.second == static_cast<u16>(ny)) {
                                already_placed = true;
                                break;
                            }
                        }
                        
                        if (!already_placed) {
                            Region* region = GetRegionAtGrid(static_cast<u16>(nx), static_cast<u16>(ny));
                            if (region && (region->GetType() == "Plains" || region->GetType() == "Forest")) {
                                if (random.RandomFloat(0.0f, 1.0f) < 0.8f) {
                                    RegionID id = region->GetID();
                                    f32 rx = region->GetX();
                                    f32 ry = region->GetY();
                                    regions_[id] = std::make_unique<Region>(id, "Rural");
                                    regions_[id]->SetPosition(rx, ry);
                                    regions_[id]->Initialize();
                                    rural_cells.push_back({static_cast<u16>(nx), static_cast<u16>(ny)});
                                    placed++;
                                    total_rural_placed++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    std::cout << "Pass 4: Placed " << capital_count << " capital clumps (" << total_urban_placed 
              << " urban cells) and " << rural_clump_count << " rural clumps (" << total_rural_placed << " rural cells)" << std::endl;
    
    // PASS 5: Expand Plains into larger areas (they're already default, but ensure continuity)
    // This is mostly handled by the neighbor influence in the final pass
    
    // PASS 6: Place Deserts in clumps in one hemisphere
    bool desert_in_north = random.RandomFloat(0.0f, 1.0f) < 0.5f;  // 50% chance for north, 50% for south
    u32 desert_clump_count = random.RandomU32(3, 7);  // 3-6 desert clumps
    u32 total_desert_placed = 0;
    
    for (u32 clump = 0; clump < desert_clump_count; ++clump) {
        u16 center_x = random.RandomU32(5, grid_width - 5);
        u16 center_y;
        if (desert_in_north) {
            center_y = random.RandomU32(5, grid_height / 2 - 5);
        } else {
            center_y = random.RandomU32(grid_height / 2 + 5, grid_height - 5);
        }
        
        // Varied desert clump size (30-70 cells) - bigger deserts
        u32 desert_size = random.RandomU32(30, 71);
        u32 placed = 0;
        
        // Clump placement with distance-based probability (larger search radius for bigger deserts)
        for (i16 dy = -12; dy <= 12 && placed < desert_size; ++dy) {
            for (i16 dx = -12; dx <= 12 && placed < desert_size; ++dx) {
                i16 nx = static_cast<i16>(center_x) + dx;
                i16 ny = static_cast<i16>(center_y) + dy;
                
                // Ensure in correct hemisphere
                if (desert_in_north && ny >= static_cast<i16>(grid_height / 2)) continue;
                if (!desert_in_north && ny < static_cast<i16>(grid_height / 2)) continue;
                
                if (nx >= 0 && nx < static_cast<i16>(grid_width) &&
                    ny >= 0 && ny < static_cast<i16>(grid_height)) {
                    Region* region = GetRegionAtGrid(static_cast<u16>(nx), static_cast<u16>(ny));
                    if (region && region->GetType() == "Plains") {
                        f32 dist = std::sqrt(static_cast<f32>(dx*dx + dy*dy));
                        f32 prob = 1.0f - (dist / 12.0f);  // Higher probability closer to center (adjusted for larger radius)
                        prob = std::max(0.0f, prob);
                        
                        if (random.RandomFloat(0.0f, 1.0f) < prob) {
                            RegionID id = region->GetID();
                            f32 dx_world = region->GetX();
                            f32 dy_world = region->GetY();
                            regions_[id] = std::make_unique<Region>(id, "Desert");
                            regions_[id]->SetPosition(dx_world, dy_world);
                            regions_[id]->Initialize();
                            placed++;
                            total_desert_placed++;
                        }
                    }
                }
            }
        }
    }
    std::cout << "Pass 6: Placed " << desert_clump_count << " desert clumps (" << total_desert_placed 
              << " desert cells) in " << (desert_in_north ? "northern" : "southern") << " hemisphere" << std::endl;
    
    // PASS 7: Place Forests in clumps (heavily weighted, not forced)
    // First, create forest clumps
    u32 forest_clump_count = random.RandomU32(5, 12);  // 5-11 forest clumps
    u32 total_forest_placed = 0;
    
    for (u32 clump = 0; clump < forest_clump_count; ++clump) {
        u16 center_x = random.RandomU32(5, grid_width - 5);
        u16 center_y = random.RandomU32(5, grid_height - 5);
        
        // Varied forest clump size (25-60 cells)
        u32 forest_size = random.RandomU32(25, 61);
        u32 placed = 0;
        
        // Clump placement with distance-based probability
        for (i16 dy = -12; dy <= 12 && placed < forest_size; ++dy) {
            for (i16 dx = -12; dx <= 12 && placed < forest_size; ++dx) {
                i16 nx = static_cast<i16>(center_x) + dx;
                i16 ny = static_cast<i16>(center_y) + dy;
                
                if (nx >= 0 && nx < static_cast<i16>(grid_width) &&
                    ny >= 0 && ny < static_cast<i16>(grid_height)) {
                    Region* region = GetRegionAtGrid(static_cast<u16>(nx), static_cast<u16>(ny));
                    if (region && region->GetType() == "Plains") {
                        f32 dist = std::sqrt(static_cast<f32>(dx*dx + dy*dy));
                        f32 prob = 1.0f - (dist / 12.0f);  // Higher probability closer to center
                        prob = std::max(0.0f, prob);
                        
                        if (random.RandomFloat(0.0f, 1.0f) < prob) {
                            RegionID id = region->GetID();
                            f32 fx = region->GetX();
                            f32 fy = region->GetY();
                            regions_[id] = std::make_unique<Region>(id, "Forest");
                            regions_[id]->SetPosition(fx, fy);
                            regions_[id]->Initialize();
                            placed++;
                            total_forest_placed++;
                        }
                    }
                }
            }
        }
    }
    std::cout << "Pass 7a: Placed " << forest_clump_count << " forest clumps (" << total_forest_placed << " forest cells)" << std::endl;
    
    // PASS 7b: Fill remaining with neighbor-influenced generation (heavily weighted towards clumping)
    // Use weighted selection based on neighbors for remaining Plains cells
    for (u16 y = 0; y < grid_height; ++y) {
        for (u16 x = 0; x < grid_width; ++x) {
            Region* region = GetRegionAtGrid(x, y);
            if (!region || region->GetType() != "Plains") {
                continue;  // Skip already assigned regions
            }
            
            // Count neighbor types
            std::unordered_map<std::string, u32> neighbor_counts;
            Region* neighbors[4] = {
                GetRegionAtGrid(x, y - 1),
                GetRegionAtGrid(x, y + 1),
                GetRegionAtGrid(x - 1, y),
                GetRegionAtGrid(x + 1, y)
            };
            
            for (Region* neighbor : neighbors) {
                if (neighbor) {
                    neighbor_counts[neighbor->GetType()]++;
                }
            }
            
            // Heavily weighted towards clumping (strong neighbor influence)
            f32 forest_weight = 0.1f + neighbor_counts["Forest"] * 8.0f + neighbor_counts["Mountain"] * 2.0f;
            f32 plains_weight = 1.0f + neighbor_counts["Plains"] * 3.0f;
            f32 desert_weight = neighbor_counts["Desert"] * 6.0f;  // Strong clumping for deserts too
            
            f32 total = forest_weight + plains_weight + desert_weight;
            f32 rand = random.RandomFloat(0.0f, total);
            
            std::string new_type = "Plains";
            if (rand < forest_weight) {
                new_type = "Forest";
            } else if (rand < forest_weight + plains_weight) {
                new_type = "Plains";  // Keep as Plains
            } else {
                new_type = "Desert";
            }
            
            if (new_type != "Plains") {
                RegionID id = region->GetID();
                f32 fx = region->GetX();
                f32 fy = region->GetY();
                regions_[id] = std::make_unique<Region>(id, new_type);
                regions_[id]->SetPosition(fx, fy);
                regions_[id]->Initialize();
            }
        }
    }
    std::cout << "Pass 7b: Filled remaining regions with neighbor-influenced clumping" << std::endl;
    
    // PASS 8: Expand coastal regions off the rim (if touching coastal)
    u32 coastal_expanded = 0;
    for (u16 y = 0; y < grid_height; ++y) {
        for (u16 x = 0; x < grid_width; ++x) {
            Region* region = GetRegionAtGrid(x, y);
            if (!region || region->GetType() == "Coastal" || IsOnRim(x, y)) {
                continue;  // Skip already coastal or rim regions
            }
            
            // Check if any neighbor is coastal
            bool has_coastal_neighbor = false;
            Region* neighbors[4] = {
                GetRegionAtGrid(x, y - 1),
                GetRegionAtGrid(x, y + 1),
                GetRegionAtGrid(x - 1, y),
                GetRegionAtGrid(x + 1, y)
            };
            
            for (Region* neighbor : neighbors) {
                if (neighbor && neighbor->GetType() == "Coastal") {
                    has_coastal_neighbor = true;
                    break;
                }
            }
            
            // 30% chance to become coastal if touching coastal (creates coastal bays/inlets)
            if (has_coastal_neighbor && random.RandomFloat(0.0f, 1.0f) < 0.3f) {
                RegionID id = region->GetID();
                f32 wx = region->GetX();
                f32 wy = region->GetY();
                regions_[id] = std::make_unique<Region>(id, "Coastal");
                regions_[id]->SetPosition(wx, wy);
                regions_[id]->Initialize();
                coastal_expanded++;
            }
        }
    }
    std::cout << "Pass 8: Expanded " << coastal_expanded << " coastal regions off the rim" << std::endl;
    
    // PASS 9: Assign subtypes based on neighbors (influences color)
    u32 subtypes_assigned = 0;
    for (u16 y = 0; y < grid_height; ++y) {
        for (u16 x = 0; x < grid_width; ++x) {
            Region* region = GetRegionAtGrid(x, y);
            if (!region) {
                continue;
            }
            
            std::string region_type = region->GetType();
            std::string subtype = "";
            
            // Count neighbor types
            std::unordered_map<std::string, u32> neighbor_counts;
            Region* neighbors[4] = {
                GetRegionAtGrid(x, y - 1),
                GetRegionAtGrid(x, y + 1),
                GetRegionAtGrid(x - 1, y),
                GetRegionAtGrid(x + 1, y)
            };
            
            for (Region* neighbor : neighbors) {
                if (neighbor) {
                    neighbor_counts[neighbor->GetType()]++;
                }
            }
            
            // Assign subtypes based on region type and neighbors
            if (region_type == "Rural" || region_type == "Urban") {
                // Inhabited areas near mountains get mountain subtype
                if (neighbor_counts["Mountain"] >= 2) {
                    subtype = "Mountain";
                } else if (neighbor_counts["Forest"] >= 2) {
                    subtype = "Forest";
                } else if (neighbor_counts["Coastal"] >= 1) {
                    subtype = "Coastal";
                }
            } else if (region_type == "Plains") {
                // Plains only get subtype if strong neighbor influence (less weighted)
                if (neighbor_counts["Mountain"] >= 3) {
                    subtype = "Mountain";
                } else if (neighbor_counts["Forest"] >= 3) {
                    subtype = "Forest";
                } else if (neighbor_counts["Desert"] >= 2) {
                    subtype = "Desert";
                } else if (neighbor_counts["Coastal"] >= 2) {
                    subtype = "Coastal";
                }
                // Otherwise, Plains stay without subtype (default)
            } else if (region_type == "Forest") {
                // Forests near mountains get mountain subtype
                if (neighbor_counts["Mountain"] >= 2) {
                    subtype = "Mountain";
                }
            } else if (region_type == "Desert") {
                // Deserts near mountains get mountain subtype (high desert)
                if (neighbor_counts["Mountain"] >= 2) {
                    subtype = "Mountain";
                }
            }
            
            // Assign subtype if determined
            if (!subtype.empty()) {
                region->SetSubtype(subtype);
                subtypes_assigned++;
            }
        }
    }
    std::cout << "Pass 9: Assigned " << subtypes_assigned << " subtypes based on neighbors" << std::endl;
    
    std::cout << "InitializeRegionGrid: Created " << regions_.size() << " regions" << std::endl;
}

} // namespace Simulation

