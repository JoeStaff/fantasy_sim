#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include "Components/Skills.h"
#include "Components/Inhabitant.h"
#include <vector>

namespace Skills {

// Skill progression system
class SkillSystem {
public:
    SkillSystem();
    ~SkillSystem();
    
    // Initialize skill system
    void Initialize();
    
    // Update skill progression for an entity
    void UpdateSkillProgression(
        Components::Skills& skills,
        const Components::Inhabitant& inhabitant,
        f32 delta_time,
        const std::vector<bool>& active_skills = {}
    );
    
    // Calculate progression probability for a skill
    f32 CalculateProgressionProbability(
        u8 current_level,
        RaceID race_id,
        SkillID skill_id,
        u16 age,
        bool is_active,
        bool is_related,
        const std::vector<f32>& event_modifiers = {}
    ) const;
    
    // Get base probability for skill level
    f32 GetBaseProbability(u8 level) const;
    
    // Get age modifier
    f32 GetAgeModifier(u16 age, RaceID race_id) const;
    
    // Check if skill can progress (mortal cap check)
    bool CanProgress(u8 current_level, bool divine_levels_enabled, u8 mortal_max_level) const;
    
private:
    Config::SkillsConfig config_;
    
    // Precomputed probability lookup table
    std::vector<f32> probability_lut_;
    
    void BuildProbabilityLUT();
    f32 InterpolateProbability(u8 level) const;
};

} // namespace Skills





