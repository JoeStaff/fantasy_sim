#pragma once

#include "Core/Types.h"
#include <cstddef>

namespace ECS {

// Entity is just an ID
using Entity = EntityID;

// Entity manager for creating and destroying entities
class EntityManager {
public:
    EntityManager();
    ~EntityManager();
    
    // Create a new entity
    Entity CreateEntity();
    
    // Destroy an entity
    void DestroyEntity(Entity entity);
    
    // Check if entity is valid
    bool IsValid(Entity entity) const;
    
    // Get total entity count
    u32 GetEntityCount() const;
    
    // Reset (clear all entities)
    void Reset();
    
private:
    EntityID next_entity_id_;
    u32 entity_count_;
};

} // namespace ECS
