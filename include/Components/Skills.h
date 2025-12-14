#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include <vector>
#include <cstring>

namespace Components {

// Skills component - stores 200 skills as 4-bit values (packed)
class Skills {
public:
    Skills();
    explicit Skills(u16 skill_count);
    ~Skills() = default;
    
    // Copy/move
    Skills(const Skills& other);
    Skills& operator=(const Skills& other);
    Skills(Skills&& other) noexcept;
    Skills& operator=(Skills&& other) noexcept;
    
    // Get skill level (0-15)
    u8 GetSkill(SkillID skill_id) const;
    
    // Set skill level (0-15)
    void SetSkill(SkillID skill_id, u8 level);
    
    // Increment skill level (with max check)
    bool IncrementSkill(SkillID skill_id, u8 max_level = 15);
    
    // Decrement skill level (with min check)
    bool DecrementSkill(SkillID skill_id, u8 min_level = 0);
    
    // Get total number of skills
    u16 GetSkillCount() const { return skill_count_; }
    
    // Get raw data (for bulk operations)
    const u8* GetData() const { return skills_data_.data(); }
    u8* GetData() { return skills_data_.data(); }
    
    // Get data size in bytes
    size_t GetDataSize() const { return skills_data_.size(); }
    
    // Reset all skills to 0
    void Reset();
    
    // Get total skill points (sum of all skills)
    u32 GetTotalSkillPoints() const;
    
    // Get highest skill level
    u8 GetHighestSkillLevel() const;
    
    // Get number of skills at or above a certain level
    u16 GetSkillsAtOrAboveLevel(u8 level) const;
    
private:
    u16 skill_count_;
    std::vector<u8> skills_data_;  // Packed: 2 skills per byte
    
    // Helper: Calculate byte index and nibble position
    void GetSkillPosition(SkillID skill_id, size_t& byte_index, bool& is_low_nibble) const;
};

} // namespace Components
