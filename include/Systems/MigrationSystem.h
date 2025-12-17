#pragma once

#include "ECS/System.h"
#include "Core/Types.h"

namespace Systems {

// Migration system - handles entity movement between regions
class MigrationSystem : public ECS::System {
public:
    MigrationSystem();
    ~MigrationSystem() override = default;
    
    void Update(f32 delta_time) override;
    
    // Check if entity should migrate
    bool ShouldMigrate(EntityID entity) const;
    
    // Migrate entity to new region
    bool MigrateEntity(EntityID entity, RegionID target_region);
    
    // Find best migration target for entity
    RegionID FindMigrationTarget(EntityID entity) const;
};

} // namespace Systems




