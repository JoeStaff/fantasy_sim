#pragma once

#include "Core/Types.h"

namespace Components {

// Renown component - sparse (only for heroes)
// Stored separately from Inhabitant for memory efficiency
struct Renown {
    u16 value = 0;  // 0 = not a hero
    
    Renown() = default;
    explicit Renown(u16 renown) : value(renown) {}
    
    bool IsHero() const { return value > 0; }
};

} // namespace Components




