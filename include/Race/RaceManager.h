#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace Race {

// Race manager - handles race definitions and lookups
class RaceManager {
public:
    static RaceManager& GetInstance();
    
    // Initialize from configuration
    bool Initialize(const Config::RacesConfig& config);
    
    // Get race definition
    const Config::RaceDefinition* GetRace(RaceID race_id) const;
    const Config::RaceDefinition* GetRaceByName(const std::string& name) const;
    
    // Get all races
    const std::vector<Config::RaceDefinition>& GetAllRaces() const;
    
    // Race-specific calculations
    f32 GetAgingRate(RaceID race_id) const;
    u16 GetMaxAge(RaceID race_id) const;
    f32 GetSkillProgressionMultiplier(RaceID race_id) const;
    f32 GetSkillAffinity(RaceID race_id, SkillID skill_id) const;
    f32 GetSkillPenalty(RaceID race_id, SkillID skill_id) const;
    
    // Regional preferences
    f32 GetRegionAttraction(RaceID race_id, const std::string& region_type) const;
    
    // Breeding
    RaceID DetermineOffspringRace(RaceID parent1, RaceID parent2) const;
    
    // Get random race (weighted by population percentage)
    RaceID GetRandomRace() const;
    
private:
    RaceManager() = default;
    ~RaceManager() = default;
    RaceManager(const RaceManager&) = delete;
    RaceManager& operator=(const RaceManager&) = delete;
    
    std::vector<Config::RaceDefinition> races_;
    std::unordered_map<std::string, RaceID> race_name_to_id_;
    Config::RacesConfig config_;
    
    void BuildRaceLookup();
};

} // namespace Race
