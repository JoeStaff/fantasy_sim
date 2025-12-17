#pragma once

#include "ECS/System.h"
#include "Core/Types.h"

namespace Systems {

// Skill progression system - handles skill leveling
class SkillProgressionSystem : public ECS::System {
public:
    SkillProgressionSystem();
    ~SkillProgressionSystem() override = default;
    
    void Update(f32 delta_time) override;
    
    // Update skills for a single entity
    void UpdateEntitySkills(EntityID entity, f32 delta_time);
    
    // Batch update (for performance)
    void BatchUpdateSkills(const std::vector<EntityID>& entities, f32 delta_time);
};

} // namespace Systems




