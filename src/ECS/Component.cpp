#include "ECS/Component.h"

namespace ECS {

ComponentManager::ComponentManager() = default;

ComponentManager::~ComponentManager() = default;

void ComponentManager::OnEntityDestroyed(EntityID entity) {
    for (auto& pair : component_arrays_) {
        pair.second->OnEntityDestroyed(entity);
    }
}

// Template specializations need to be in header, but we can keep non-template methods here

} // namespace ECS




