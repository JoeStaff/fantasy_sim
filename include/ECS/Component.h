#pragma once

#include "Core/Types.h"
#include <cstddef>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <vector>

namespace ECS {

// Component type ID
using ComponentTypeID = std::type_index;

// Base component interface
struct IComponent {
    virtual ~IComponent() = default;
};

// Component manager for storing components in SoA format
class ComponentManager {
public:
    ComponentManager();
    ~ComponentManager();
    
    // Register a component type
    template<typename T>
    void RegisterComponent();
    
    // Add component to entity
    template<typename T>
    void AddComponent(EntityID entity, const T& component);
    
    // Remove component from entity
    template<typename T>
    void RemoveComponent(EntityID entity);
    
    // Get component from entity
    template<typename T>
    T* GetComponent(EntityID entity);
    
    template<typename T>
    const T* GetComponent(EntityID entity) const;
    
    // Check if entity has component
    template<typename T>
    bool HasComponent(EntityID entity) const;
    
    // Get component type ID
    template<typename T>
    static ComponentTypeID GetComponentTypeID();
    
    // Entity destroyed callback
    void OnEntityDestroyed(EntityID entity);
    
private:
    // Component array interface
    struct IComponentArray {
        virtual ~IComponentArray() = default;
        virtual void OnEntityDestroyed(EntityID entity) = 0;
    };
    
    template<typename T>
    class ComponentArray : public IComponentArray {
    public:
        void InsertData(EntityID entity, const T& component);
        void RemoveData(EntityID entity);
        T& GetData(EntityID entity);
        const T& GetData(EntityID entity) const;
        bool HasData(EntityID entity) const;
        void OnEntityDestroyed(EntityID entity) override;
        
    private:
        // SoA storage
        std::vector<T> component_array_;
        std::unordered_map<EntityID, size_t> entity_to_index_;
        std::unordered_map<size_t, EntityID> index_to_entity_;
    };
    
    // Get component array (internal helper)
    template<typename T>
    ComponentArray<T>* GetComponentArray();
    
    template<typename T>
    const ComponentArray<T>* GetComponentArray() const;
    
    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays_;
    std::unordered_map<ComponentTypeID, const char*> component_types_;
};

// Template implementations
template<typename T>
void ComponentManager::RegisterComponent() {
    ComponentTypeID type_id = GetComponentTypeID<T>();
    if (component_arrays_.find(type_id) == component_arrays_.end()) {
        component_arrays_[type_id] = std::make_unique<ComponentArray<T>>();
    }
}

template<typename T>
void ComponentManager::AddComponent(EntityID entity, const T& component) {
    GetComponentArray<T>()->InsertData(entity, component);
}

template<typename T>
void ComponentManager::RemoveComponent(EntityID entity) {
    GetComponentArray<T>()->RemoveData(entity);
}

template<typename T>
T* ComponentManager::GetComponent(EntityID entity) {
    auto* array = GetComponentArray<T>();
    if (array && array->HasData(entity)) {
        return &array->GetData(entity);
    }
    return nullptr;
}

template<typename T>
const T* ComponentManager::GetComponent(EntityID entity) const {
    const auto* array = GetComponentArray<T>();
    if (array && array->HasData(entity)) {
        return &array->GetData(entity);
    }
    return nullptr;
}

template<typename T>
ComponentManager::ComponentArray<T>* ComponentManager::GetComponentArray() {
    ComponentTypeID type_id = GetComponentTypeID<T>();
    auto it = component_arrays_.find(type_id);
    if (it != component_arrays_.end()) {
        return static_cast<ComponentArray<T>*>(it->second.get());
    }
    return nullptr;
}

template<typename T>
const ComponentManager::ComponentArray<T>* ComponentManager::GetComponentArray() const {
    ComponentTypeID type_id = GetComponentTypeID<T>();
    auto it = component_arrays_.find(type_id);
    if (it != component_arrays_.end()) {
        return static_cast<const ComponentArray<T>*>(it->second.get());
    }
    return nullptr;
}

template<typename T>
bool ComponentManager::HasComponent(EntityID entity) const {
    const auto* array = GetComponentArray<T>();
    return array && array->HasData(entity);
}

template<typename T>
ComponentTypeID ComponentManager::GetComponentTypeID() {
    return std::type_index(typeid(T));
}

// ComponentArray template implementations
template<typename T>
void ComponentManager::ComponentArray<T>::InsertData(EntityID entity, const T& component) {
    if (entity_to_index_.find(entity) != entity_to_index_.end()) {
        // Entity already has component, update it
        size_t index = entity_to_index_[entity];
        component_array_[index] = component;
        return;
    }
    
    size_t new_index = component_array_.size();
    component_array_.push_back(component);
    entity_to_index_[entity] = new_index;
    index_to_entity_[new_index] = entity;
}

template<typename T>
void ComponentManager::ComponentArray<T>::RemoveData(EntityID entity) {
    auto it = entity_to_index_.find(entity);
    if (it == entity_to_index_.end()) {
        return;
    }
    
    size_t index_to_remove = it->second;
    size_t last_index = component_array_.size() - 1;
    
    // Move last element to removed position
    EntityID last_entity = index_to_entity_[last_index];
    component_array_[index_to_remove] = component_array_[last_index];
    
    // Update mappings
    entity_to_index_[last_entity] = index_to_remove;
    index_to_entity_[index_to_remove] = last_entity;
    
    // Remove old mappings
    entity_to_index_.erase(entity);
    index_to_entity_.erase(last_index);
    
    // Remove last element
    component_array_.pop_back();
}

template<typename T>
T& ComponentManager::ComponentArray<T>::GetData(EntityID entity) {
    size_t index = entity_to_index_[entity];
    return component_array_[index];
}

template<typename T>
const T& ComponentManager::ComponentArray<T>::GetData(EntityID entity) const {
    size_t index = entity_to_index_.at(entity);
    return component_array_[index];
}

template<typename T>
bool ComponentManager::ComponentArray<T>::HasData(EntityID entity) const {
    return entity_to_index_.find(entity) != entity_to_index_.end();
}

template<typename T>
void ComponentManager::ComponentArray<T>::OnEntityDestroyed(EntityID entity) {
    if (HasData(entity)) {
        RemoveData(entity);
    }
}

} // namespace ECS




