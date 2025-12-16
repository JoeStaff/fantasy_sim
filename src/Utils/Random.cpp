#include "Utils/Random.h"
#include <random>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <iterator>
#include <vector>
#include <string>

namespace Utils {

Random& Random::GetInstance() {
    static Random instance;
    return instance;
}

Random::Random()
    : generator_(std::random_device{}())
    , float_dist_(0.0f, 1.0f)
    , u32_dist_(0, std::numeric_limits<u32>::max())
    , u64_dist_(0, std::numeric_limits<u64>::max())
{
    // Seed with current time
    auto now = std::chrono::high_resolution_clock::now();
    auto seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    generator_.seed(static_cast<std::mt19937::result_type>(seed));
}

void Random::Seed(u64 seed) {
    generator_.seed(static_cast<std::mt19937::result_type>(seed));
}

void Random::Seed() {
    auto now = std::chrono::high_resolution_clock::now();
    auto seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    generator_.seed(static_cast<std::mt19937::result_type>(seed));
}

f32 Random::RandomFloat() {
    return float_dist_(generator_);
}

f32 Random::RandomFloat(f32 min, f32 max) {
    std::uniform_real_distribution<f32> dist(min, max);
    return dist(generator_);
}

u32 Random::RandomU32() {
    return u32_dist_(generator_);
}

u32 Random::RandomU32(u32 min, u32 max) {
    std::uniform_int_distribution<u32> dist(min, max);
    return dist(generator_);
}

u64 Random::RandomU64() {
    return u64_dist_(generator_);
}

i32 Random::RandomI32(i32 min, i32 max) {
    std::uniform_int_distribution<i32> dist(min, max);
    return dist(generator_);
}

bool Random::RandomBool(f32 probability) {
    return RandomFloat() < probability;
}

template<typename Container>
auto Random::RandomChoice(const Container& container) -> decltype(*container.begin()) {
    if (container.empty()) {
        throw std::runtime_error("Random::RandomChoice called on empty container");
    }
    
    u32 index = RandomU32(0, static_cast<u32>(container.size() - 1));
    auto it = container.begin();
    std::advance(it, index);
    return *it;
}

} // namespace Utils

