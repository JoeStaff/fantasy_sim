#pragma once

#include <cstdint>
#include <cstddef>

// Type aliases for consistency
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

// Entity ID type
using EntityID = u64;
constexpr EntityID INVALID_ENTITY_ID = 0;

// Region ID type (supports up to 4 billion regions)
using RegionID = u32;
constexpr RegionID INVALID_REGION_ID = 0xFFFFFFFF;

// Race ID type (max 255 races)
using RaceID = u8;
constexpr RaceID INVALID_RACE_ID = 255;

// Skill ID type
using SkillID = u16;

// Tick type for simulation time
using Tick = u64;

// Simulation LOD levels
enum class SimulationLOD : u8 {
    Full = 0,      // Full individual simulation
    Half = 1,      // Reduced detail simulation
    Formula = 2    // Statistical/formula-based simulation
};

