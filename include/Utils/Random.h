#pragma once

#include "Core/Types.h"
#include <random>
#include <memory>

namespace Utils {

// Thread-safe random number generator
class Random {
public:
    static Random& GetInstance();
    
    // Initialize with seed
    void Seed(u64 seed);
    void Seed();
    
    // Generate random numbers
    f32 RandomFloat();  // [0.0, 1.0)
    f32 RandomFloat(f32 min, f32 max);  // [min, max)
    u32 RandomU32();
    u32 RandomU32(u32 min, u32 max);  // [min, max]
    u64 RandomU64();
    i32 RandomI32(i32 min, i32 max);  // [min, max]
    
    // Probability check
    bool RandomBool(f32 probability);  // Returns true with given probability
    
    // Random selection from container
    template<typename Container>
    auto RandomChoice(const Container& container) -> decltype(*container.begin());
    
private:
    Random();
    ~Random() = default;
    Random(const Random&) = delete;
    Random& operator=(const Random&) = delete;
    
    std::mt19937 generator_;
    std::uniform_real_distribution<f32> float_dist_;
    std::uniform_int_distribution<u32> u32_dist_;
    std::uniform_int_distribution<u64> u64_dist_;
};

} // namespace Utils
