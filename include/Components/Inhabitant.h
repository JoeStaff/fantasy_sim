#pragma once

#include "Core/Types.h"
#include <cstddef>

namespace Components {

// Inhabitant component - core entity data
struct Inhabitant {
    EntityID id = INVALID_ENTITY_ID;
    RegionID region_id = INVALID_REGION_ID;
    RaceID race_id = INVALID_RACE_ID;
    u16 age = 0;
    
    // Traits bitfield (packed flags)
    // Bit 0: is_hero
    // Bit 1-7: reserved for future traits
    u8 traits = 0;
    
    // Helper methods
    bool IsHero() const { return (traits & 0x01) != 0; }
    void SetHero(bool is_hero) {
        if (is_hero) {
            traits |= 0x01;
        } else {
            traits &= ~0x01;
        }
    }
};

} // namespace Components




