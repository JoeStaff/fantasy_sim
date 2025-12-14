#include "ECS/Entity.h"

namespace ECS {

EntityManager::EntityManager() 
    : next_entity_id_(1)
    , entity_count_(0) {
}

EntityManager::~EntityManager() = default;

Entity EntityManager::CreateEntity() {
    entity_count_++;
    return next_entity_id_++;
}

void EntityManager::DestroyEntity(Entity entity) {
    if (IsValid(entity)) {
        entity_count_--;
    }
}

bool EntityManager::IsValid(Entity entity) const {
    return entity != INVALID_ENTITY_ID && entity < next_entity_id_;
}

u32 EntityManager::GetEntityCount() const {
    return entity_count_;
}

void EntityManager::Reset() {
    next_entity_id_ = 1;
    entity_count_ = 0;
}

} // namespace ECS
