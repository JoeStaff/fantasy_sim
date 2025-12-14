#pragma once

#include "Core/Types.h"
#include "Components/Inhabitant.h"
#include "Components/Skills.h"
#include "Components/Transform.h"
#include <vector>
#include <memory>

namespace Data {

// Entity storage using SoA (Structure of Arrays) pattern
class EntityStorage {
public:
    EntityStorage();
    ~EntityStorage();
    
    // Initialize storage
    void Initialize(u32 max_entities);
    
    // Add entity
    EntityID AddEntity();
    
    // Remove entity
    void RemoveEntity(EntityID entity);
    
    // Get component arrays (for batch processing)
    std::vector<Components::Inhabitant>& GetInhabitants() { return inhabitants_; }
    std::vector<Components::Skills>& GetSkills() { return skills_; }
    std::vector<Components::Transform>& GetTransforms() { return transforms_; }
    
    const std::vector<Components::Inhabitant>& GetInhabitants() const { return inhabitants_; }
    const std::vector<Components::Skills>& GetSkills() const { return skills_; }
    const std::vector<Components::Transform>& GetTransforms() const { return transforms_; }
    
    // Get entity count
    u32 GetEntityCount() const { return entity_count_; }
    
    // Get max entities
    u32 GetMaxEntities() const { return max_entities_; }
    
    // Resize storage
    void Resize(u32 new_size);
    
    // Clear all entities
    void Clear();
    
private:
    u32 max_entities_ = 0;
    u32 entity_count_ = 0;
    
    // SoA storage
    std::vector<Components::Inhabitant> inhabitants_;
    std::vector<Components::Skills> skills_;
    std::vector<Components::Transform> transforms_;
    
    // Entity ID to index mapping
    std::unordered_map<EntityID, u32> entity_to_index_;
    std::vector<EntityID> index_to_entity_;
    std::vector<bool> valid_entities_;
    
    EntityID next_entity_id_ = 1;
};

} // namespace Data
