#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include "Components/Hero.h"
#include "Components/Renown.h"
#include "Components/Inhabitant.h"
#include "Components/Skills.h"
#include <vector>
#include <unordered_map>

namespace Heroes {

// Hero system - manages heroes and renown
class HeroSystem {
public:
    HeroSystem();
    ~HeroSystem();
    
    // Initialize hero system
    void Initialize();
    
    // Update hero system
    void Update(f32 delta_time, Tick current_tick);
    
    // Promote entity to hero (if renown threshold met)
    bool CheckAndPromote(EntityID entity, u16 new_renown);
    
    // Award renown to entity
    void AwardRenown(EntityID entity, u16 amount, const std::string& source);
    
    // Get hero data
    Components::Hero* GetHero(EntityID entity);
    const Components::Hero* GetHero(EntityID entity) const;
    
    // Check if entity is hero
    bool IsHero(EntityID entity) const;
    
    // Get all hero IDs
    std::vector<EntityID> GetAllHeroes() const;
    
    // Calculate renown from skills
    u16 CalculateRenownFromSkills(const Components::Skills& skills) const;
    
    // Award renown for combat achievement
    void AwardCombatRenown(EntityID entity, const std::string& achievement_type, u16 base_amount);
    
    // Award renown for skill milestone
    void AwardSkillRenown(EntityID entity, SkillID skill_id, u8 level);
    
    // Award renown for lineage
    void AwardLineageRenown(EntityID entity, EntityID parent1, EntityID parent2);
    
    // Award renown for accomplishment
    void AwardAccomplishmentRenown(EntityID entity, const std::string& accomplishment_type);
    
    // Update hero influences on regions
    void UpdateHeroInfluences();
    
    // Get hero tier
    const char* GetHeroTier(u16 renown) const;
    
    // Get influence radius for renown level
    u8 GetInfluenceRadius(u16 renown) const;
    
    // Calculate influence strength
    f32 CalculateInfluenceStrength(u16 renown, u8 distance_in_regions) const;
    
private:
    Config::HeroesConfig config_;
    std::unordered_map<EntityID, Components::Hero> heroes_;
    
    void UpdateRenownDecay(Tick current_tick);
    void UpdateInfluenceForHero(EntityID hero_id);
    u8 DetermineHeroType(const Components::Skills& skills) const;
};

} // namespace Heroes




