#include "Simulation/World.h"
#include <algorithm>

namespace Simulation {

World::World() = default;

World::~World() = default;

void World::Initialize(u16 grid_width, u16 grid_height, f32 region_size) {
    grid_width_ = grid_width;
    grid_height_ = grid_height;
    region_size_ = region_size;
    
    // Clear existing data
    regions_.clear();
    settlements_.clear();
    roads_.clear();
    
    // Calculate total regions
    u32 total_regions = static_cast<u32>(grid_width) * static_cast<u32>(grid_height);
    regions_.reserve(total_regions);
}

Region* World::GetRegion(RegionID region_id) {
    for (auto& region : regions_) {
        if (region && region->GetID() == region_id) {
            return region.get();
        }
    }
    return nullptr;
}

const Region* World::GetRegion(RegionID region_id) const {
    for (const auto& region : regions_) {
        if (region && region->GetID() == region_id) {
            return region.get();
        }
    }
    return nullptr;
}

Region* World::GetRegionAtGrid(u16 grid_x, u16 grid_y) {
    if (grid_x >= grid_width_ || grid_y >= grid_height_) {
        return nullptr;
    }
    u32 idx = static_cast<u32>(grid_y) * static_cast<u32>(grid_width_) + static_cast<u32>(grid_x);
    if (idx < regions_.size()) {
        return regions_[idx].get();
    }
    return nullptr;
}

const Region* World::GetRegionAtGrid(u16 grid_x, u16 grid_y) const {
    if (grid_x >= grid_width_ || grid_y >= grid_height_) {
        return nullptr;
    }
    u32 idx = static_cast<u32>(grid_y) * static_cast<u32>(grid_width_) + static_cast<u32>(grid_x);
    if (idx < regions_.size()) {
        return regions_[idx].get();
    }
    return nullptr;
}

void World::AddSettlement(const Settlement& settlement) {
    settlements_.push_back(settlement);
}

void World::AddRoad(const Road& road) {
    roads_.push_back(road);
}

void World::AddSourceRegion(RegionID region_id) {
    source_regions_.push_back(region_id);
}

std::vector<RegionID> World::GetRegionsForSource(RegionID source_id) const {
    std::vector<RegionID> result;
    result.push_back(source_id);  // Include source itself
    
    for (const auto& region : regions_) {
        if (region && region->GetSourceParentID() == source_id) {
            result.push_back(region->GetID());
        }
    }
    
    return result;
}

} // namespace Simulation



