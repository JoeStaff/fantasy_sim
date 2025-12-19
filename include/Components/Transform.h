#pragma once

#include "Core/Types.h"

namespace Components {

// Transform component (position in world)
struct Transform {
    f32 x = 0.0f;
    f32 y = 0.0f;
    
    Transform() = default;
    Transform(f32 x, f32 y) : x(x), y(y) {}
};

} // namespace Components





