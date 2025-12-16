#pragma once

#include "Core/Types.h"
#include <vector>

namespace Components {

// Hero component - only for entities with renown > 0
struct Hero {
    EntityID entity_id = INVALID_ENTITY_ID;
    u16 renown = 0;
    u8 influence_radius = 0;
    u8 hero_type = 0;  // 0=Combat, 1=Crafting, 2=Knowledge, 3=Social, 4=Mixed
    
    // Influenced regions (sparse, updated periodically)
    std::vector<RegionID> influenced_regions;
    std::vector<f32> influence_strengths;
    
    // Timestamps
    Tick last_accomplishment_tick = 0;
    Tick promotion_tick = 0;
    
    // Helper methods
    bool IsLocalHero() const;
    bool IsRegionalHero() const;
    bool IsNationalHero() const;
    bool IsLegendaryHero() const;
    
    // Get hero tier name
    const char* GetTierName() const;
};

} // namespace Components



