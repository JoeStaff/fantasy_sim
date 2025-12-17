#include "ECS/System.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"

namespace ECS {

SystemManager::SystemManager() = default;

SystemManager::~SystemManager() = default;

void SystemManager::OnEntityDestroyed(EntityID entity) {
    // Notify all systems
    for (auto& pair : systems_) {
        pair.second->OnEntityDestroyed(entity);
    }
}

void SystemManager::OnEntitySignatureChanged(EntityID entity, Signature signature) {
    (void)entity;  // Suppress unused parameter warning
    // Notify systems if entity matches their signature
    for (auto& pair : systems_) {
        const Signature& system_signature = system_signatures_[pair.first];
        if ((signature & system_signature) == system_signature) {
            // Entity matches system signature
        }
    }
}

void SystemManager::Update(f32 delta_time) {
    for (auto& pair : systems_) {
        pair.second->Update(delta_time);
    }
}

Coordinator& Coordinator::GetInstance() {
    static Coordinator instance;
    return instance;
}

EntityID Coordinator::CreateEntity() {
    if (!entity_manager_) {
        entity_manager_ = std::make_unique<EntityManager>();
    }
    return entity_manager_->CreateEntity();
}

void Coordinator::DestroyEntity(EntityID entity) {
    if (entity_manager_) {
        entity_manager_->DestroyEntity(entity);
    }
    if (component_manager_) {
        component_manager_->OnEntityDestroyed(entity);
    }
    if (system_manager_) {
        system_manager_->OnEntityDestroyed(entity);
    }
}

void Coordinator::Update(f32 delta_time) {
    if (system_manager_) {
        system_manager_->Update(delta_time);
    }
}

} // namespace ECS




