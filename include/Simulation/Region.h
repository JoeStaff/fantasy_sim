#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Simulation {

// Forward declarations
class EntityManager;

// Region class
class Region {
public:
    Region(RegionID id, const std::string& type);
    ~Region();
    
    // Initialize region
    void Initialize();
    
    // Update region (called based on LOD)
    void Update(f32 delta_time, SimulationLOD lod, Tick current_tick);
    
    // Getters
    RegionID GetID() const { return id_; }
    const std::string& GetType() const { return type_; }
    const std::string& GetSubtype() const { return subtype_; }
    void SetSubtype(const std::string& subtype) { subtype_ = subtype; }
    u32 GetPopulation() const { return population_count_; }
    u32 GetCapacity() const { return capacity_; }
    
    // Population management
    void AddEntity(EntityID entity);
    void RemoveEntity(EntityID entity);
    bool IsAtCapacity() const;
    
    // Resources
    f32 GetResource(const std::string& resource_type) const;
    void SetResource(const std::string& resource_type, f32 value);
    void ModifyResource(const std::string& resource_type, f32 delta);
    
    // Traits
    void SetTrait(u8 category, u8 trait_id, bool value);
    bool GetTrait(u8 category, u8 trait_id) const;
    
    // Neighbors
    void AddNeighbor(RegionID neighbor_id);
    const std::vector<RegionID>& GetNeighbors() const { return neighbors_; }
    
    // Position
    void SetPosition(f32 x, f32 y);
    f32 GetX() const { return x_; }
    f32 GetY() const { return y_; }
    
    // Hero influences
    void AddHeroInfluence(EntityID hero_id, f32 strength);
    void RemoveHeroInfluence(EntityID hero_id);
    f32 GetHeroInfluence(EntityID hero_id) const;
    const std::unordered_map<EntityID, f32>& GetHeroInfluences() const;
    
    // Skill distributions (for formula simulation)
    void UpdateSkillDistribution(SkillID skill_id, f32 mean, f32 std_dev);
    f32 GetSkillMean(SkillID skill_id) const;
    f32 GetSkillStdDev(SkillID skill_id) const;
    
private:
    RegionID id_;
    std::string type_;
    std::string subtype_;  // Influences color (e.g., "Mountain" for rural near mountains)
    u32 population_count_ = 0;
    u32 capacity_ = 10000;
    
    // Resources
    std::unordered_map<std::string, f32> resources_;
    
    // Traits (bitfield per category)
    std::vector<u64> traits_;  // 5 categories, each with bitfield
    
    // Neighbors
    std::vector<RegionID> neighbors_;
    
    // Position
    f32 x_ = 0.0f;
    f32 y_ = 0.0f;
    
    // Hero influences
    std::unordered_map<EntityID, f32> hero_influences_;
    
    // Skill distributions (for formula simulation)
    std::vector<f32> skill_means_;
    std::vector<f32> skill_std_devs_;
    
    // Update methods by LOD
    void UpdateFullSimulation(f32 delta_time);
    void UpdateHalfSimulation(f32 delta_time);
    void UpdateFormulaSimulation(f32 delta_time);
};

} // namespace Simulation

