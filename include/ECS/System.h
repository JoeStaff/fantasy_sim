#pragma once

#include "Core/Types.h"
#include "ECS/Component.h"
#include "ECS/Entity.h"
#include <bitset>
#include <vector>
#include <memory>
#include <unordered_map>

namespace ECS {

// Maximum number of component types
constexpr size_t MAX_COMPONENTS = 64;

// Signature: bitset indicating which components an entity has
using Signature = std::bitset<MAX_COMPONENTS>;

// Base system interface
class System {
public:
    virtual ~System() = default;
    
    // Update system
    virtual void Update(f32 delta_time) = 0;
    
    // Entity destroyed callback (optional override)
    virtual void OnEntityDestroyed(EntityID entity) { (void)entity; }
    
    // Get required component signature
    Signature GetSignature() const { return signature_; }
    
protected:
    Signature signature_;
    
    // Add required component type
    template<typename T>
    void RequireComponent();
};

// System manager
class SystemManager {
public:
    SystemManager();
    ~SystemManager();
    
    // Register a system
    template<typename T>
    std::shared_ptr<T> RegisterSystem();
    
    // Set component signature for a system
    template<typename T>
    void SetSignature(Signature signature);
    
    // Entity destroyed callback
    void OnEntityDestroyed(EntityID entity);
    
    // Entity signature changed callback
    void OnEntitySignatureChanged(EntityID entity, Signature signature);
    
    // Update all systems
    void Update(f32 delta_time);
    
private:
    std::unordered_map<const char*, Signature> system_signatures_;
    std::unordered_map<const char*, std::shared_ptr<System>> systems_;
};

// Coordinator: Main ECS interface
class Coordinator {
public:
    static Coordinator& GetInstance();
    
    // Entity management
    EntityID CreateEntity();
    void DestroyEntity(EntityID entity);
    
    // Component management
    template<typename T>
    void RegisterComponent();
    
    template<typename T>
    void AddComponent(EntityID entity, const T& component);
    
    template<typename T>
    void RemoveComponent(EntityID entity);
    
    template<typename T>
    T* GetComponent(EntityID entity);
    
    template<typename T>
    bool HasComponent(EntityID entity);
    
    // System management
    template<typename T>
    std::shared_ptr<T> RegisterSystem();
    
    template<typename T>
    void SetSystemSignature(Signature signature);
    
    // Update all systems
    void Update(f32 delta_time);
    
private:
    Coordinator() = default;
    ~Coordinator() = default;
    Coordinator(const Coordinator&) = delete;
    Coordinator& operator=(const Coordinator&) = delete;
    
    std::unique_ptr<EntityManager> entity_manager_;
    std::unique_ptr<ComponentManager> component_manager_;
    std::unique_ptr<SystemManager> system_manager_;
};

// Template implementations
template<typename T>
void System::RequireComponent() {
    // This would need component type ID system - simplified for now
}

template<typename T>
std::shared_ptr<T> SystemManager::RegisterSystem() {
    const char* type_name = typeid(T).name();
    
    auto it = systems_.find(type_name);
    if (it != systems_.end()) {
        return std::static_pointer_cast<T>(it->second);
    }
    
    auto system = std::make_shared<T>();
    systems_[type_name] = system;
    return system;
}

template<typename T>
void SystemManager::SetSignature(Signature signature) {
    const char* type_name = typeid(T).name();
    system_signatures_[type_name] = signature;
}

template<typename T>
void Coordinator::RegisterComponent() {
    if (!component_manager_) {
        component_manager_ = std::make_unique<ComponentManager>();
    }
    component_manager_->RegisterComponent<T>();
}

template<typename T>
void Coordinator::AddComponent(EntityID entity, const T& component) {
    if (!component_manager_) {
        component_manager_ = std::make_unique<ComponentManager>();
    }
    component_manager_->AddComponent<T>(entity, component);
}

template<typename T>
void Coordinator::RemoveComponent(EntityID entity) {
    if (component_manager_) {
        component_manager_->RemoveComponent<T>(entity);
    }
}

template<typename T>
T* Coordinator::GetComponent(EntityID entity) {
    if (!component_manager_) {
        return nullptr;
    }
    return component_manager_->GetComponent<T>(entity);
}

template<typename T>
bool Coordinator::HasComponent(EntityID entity) {
    if (!component_manager_) {
        return false;
    }
    return component_manager_->HasComponent<T>(entity);
}

template<typename T>
std::shared_ptr<T> Coordinator::RegisterSystem() {
    if (!system_manager_) {
        system_manager_ = std::make_unique<SystemManager>();
    }
    return system_manager_->RegisterSystem<T>();
}

template<typename T>
void Coordinator::SetSystemSignature(Signature signature) {
    if (!system_manager_) {
        system_manager_ = std::make_unique<SystemManager>();
    }
    system_manager_->SetSignature<T>(signature);
}

} // namespace ECS



