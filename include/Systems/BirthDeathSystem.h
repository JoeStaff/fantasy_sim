#pragma once

#include "ECS/System.h"
#include "Core/Types.h"

namespace Systems {

// Birth and death system
class BirthDeathSystem : public ECS::System {
public:
    BirthDeathSystem();
    ~BirthDeathSystem() override = default;
    
    void Update(f32 delta_time) override;
    
    // Create new entity (birth)
    EntityID CreateNewEntity(RegionID region_id, RaceID race_id, EntityID parent1 = INVALID_ENTITY_ID, EntityID parent2 = INVALID_ENTITY_ID);
    
    // Process births for a region
    void ProcessBirths(RegionID region_id, f32 delta_time);
    
    // Process deaths for a region
    void ProcessDeaths(RegionID region_id, f32 delta_time);
};

} // namespace Systems




