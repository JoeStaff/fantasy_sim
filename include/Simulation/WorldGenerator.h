#pragma once

#include "Core/Types.h"
#include "Simulation/World.h"
#include <memory>

namespace Simulation {

// Abstract base class for world generators
class WorldGenerator {
public:
    WorldGenerator() = default;
    virtual ~WorldGenerator() = default;
    
    // Generate a world with the given dimensions
    virtual std::unique_ptr<World> Generate(u16 grid_width, u16 grid_height, f32 region_size) = 0;
    
protected:
    // Helper to check if position is on the rim (outer edge)
    static bool IsOnRim(u16 x, u16 y, u16 grid_width, u16 grid_height) {
        return (x == 0 || x == grid_width - 1 || y == 0 || y == grid_height - 1);
    }
    
    // Helper to check distance from rim
    static u16 DistanceFromRim(u16 x, u16 y, u16 grid_width, u16 grid_height) {
        u16 dist_x = std::min(x, static_cast<u16>(grid_width - 1 - x));
        u16 dist_y = std::min(y, static_cast<u16>(grid_height - 1 - y));
        return std::min(dist_x, dist_y);
    }
    
    // Helper to check if in northern hemisphere
    static bool IsInNorthernHemisphere(u16 y, u16 grid_height) {
        return y < grid_height / 2;
    }
};

} // namespace Simulation



