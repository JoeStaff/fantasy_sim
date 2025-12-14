#pragma once

#include "ECS/System.h"
#include "Core/Types.h"

namespace Systems {

// Aging system - handles entity aging and death
class AgingSystem : public ECS::System {
public:
    AgingSystem();
    ~AgingSystem() override = default;
    
    void Update(f32 delta_time) override;
    
    // Check if entity should age this tick
    bool ShouldAge(EntityID entity, f32 delta_time) const;
    
    // Age an entity
    void AgeEntity(EntityID entity, f32 delta_time);
    
    // Check if entity should die
    bool ShouldDie(EntityID entity) const;
    
    // Kill an entity
    void KillEntity(EntityID entity);
};

} // namespace Systems
