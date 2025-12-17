#pragma once

#include "Types.h"
#include "Simulation/RegionDefinition.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Config {

// World configuration
struct WorldConfig {
    u32 max_population = 1000000;
    u32 initial_population = 10000;
    u8 region_count = 100;
    f32 region_size = 1000.0f;
    f32 world_width = 10000.0f;
    f32 world_height = 10000.0f;
    f32 time_scale = 1.0f;
    f32 tick_rate = 60.0f;
    u16 region_grid_width = 100;   // Grid width for region layout
    u16 region_grid_height = 100;  // Grid height for region layout
};

// Performance configuration
struct PerformanceConfig {
    f32 target_fps = 60.0f;
    f32 target_frame_time_ms = 16.67f;
    u32 thread_count = 0;  // 0 = auto-detect
    bool thread_count_override = false;
    u32 batch_size = 128;
    bool simd_enabled = true;
    bool parallel_processing = true;
    u32 cache_line_size = 64;
    u32 memory_pool_size_mb = 2048;
    bool enable_profiling = false;
};

// Simulation configuration
struct SimulationConfig {
    struct LODConfig {
        u8 focus_region_count = 3;
        u8 visible_region_count = 10;
        u8 neighbor_range = 1;  // Range of neighbors around selected region for full simulation
        u32 full_sim_update_frequency = 1;
        u32 half_sim_update_frequency = 3;
        u32 formula_sim_update_frequency = 30;
        f32 lod_transition_smoothness = 0.5f;
        bool auto_focus_enabled = true;
    } lod;
    
    struct EntityConfig {
        u16 max_age = 120;
        f32 birth_rate_base = 0.02f;
        f32 death_rate_base = 0.01f;
        f32 aging_rate = 1.0f;
        bool enable_aging = true;
    } entity;
    
    struct RegionConfig {
        f32 max_capacity_multiplier = 1.5f;
        f32 resource_regeneration_rate = 0.1f;
        bool migration_enabled = true;
        f32 migration_rate = 0.001f;
    } region;
};

// Skills configuration
struct SkillsConfig {
    u16 skill_count = 200;
    u8 max_skill_level = 15;
    u8 min_skill_level = 0;
    u8 mortal_max_level = 9;
    bool divine_levels_enabled = false;
    u8 divine_level_min = 10;
    u8 divine_level_max = 15;
    
    struct ProgressionConfig {
        f32 base_probability_level_0 = 0.1f;
        f32 base_probability_level_5 = 0.01f;
        f32 base_probability_level_8 = 0.001f;
        f32 base_probability_level_9 = 0.0001f;
        f32 base_probability_level_10 = 0.00001f;
        f32 base_probability_level_14 = 0.000001f;
        f32 progression_curve_exponent = 2.0f;
        f32 activity_multiplier_active = 10.0f;
        f32 activity_multiplier_related = 2.0f;
        f32 activity_multiplier_inactive = 0.1f;
        f32 age_modifier_childhood = 1.5f;
        f32 age_modifier_adolescence = 1.2f;
        f32 age_modifier_prime = 1.0f;
        f32 age_modifier_middle_age = 0.8f;
        f32 age_modifier_elder = 0.9f;
        bool enable_skill_decay = false;
        f32 decay_probability = 0.0001f;
    } progression;
    
    struct HeroPromotionConfig {
        u8 skill_milestone_level = 6;
        u16 renown_per_level_9 = 10;
        u16 renown_per_additional_level_9 = 5;
        u16 renown_per_divine_level = 50;
        f32 top_percentile_threshold = 0.001f;
        u16 renown_top_percentile_min = 25;
        u16 renown_top_percentile_max = 100;
    } hero_promotion;
};

// Heroes configuration
struct HeroesConfig {
    u32 max_heroes = 1000;
    f32 hero_percentage = 0.001f;
    
    struct RenownConfig {
        u16 min_renown = 1;
        u16 max_renown = 65535;
        u16 local_hero_threshold = 100;
        u16 regional_hero_threshold = 1000;
        u16 national_hero_threshold = 10000;
        u16 legendary_hero_threshold = 10001;
        bool decay_enabled = false;
        f32 decay_rate_per_year = 1.0f;
        f32 decay_inactivity_years = 5.0f;
    } renown;
    
    struct InfluenceConfig {
        u8 local_hero_radius = 2;
        u8 regional_hero_radius = 5;
        u8 national_hero_radius = 10;
        u8 legendary_hero_radius = 20;
        u32 influence_update_frequency = 10;
        f32 distance_modifier_exponent = 1.0f;
        f32 base_influence_divisor = 1000.0f;
    } influence;
    
    struct RenownSourcesConfig {
        struct CombatConfig {
            u16 monster_defeat_min = 50;
            u16 monster_defeat_max = 500;
            u16 criminal_defeat_min = 25;
            u16 criminal_defeat_max = 200;
            u16 battle_win_min = 100;
            u16 battle_win_max = 1000;
            u16 region_protection_min = 10;
            u16 region_protection_max = 100;
        } combat;
        
        struct SkillConfig {
            u16 level_15_renown = 10;
            u16 additional_level_15_renown = 5;
            u16 top_percentile_renown_min = 25;
            u16 top_percentile_renown_max = 100;
        } skill;
        
        struct LineageConfig {
            u16 child_of_hero_min = 5;
            u16 child_of_hero_max = 50;
            u16 descendant_legendary_min = 10;
            u16 descendant_legendary_max = 100;
            f32 inheritance_percentage_min = 0.10f;
            f32 inheritance_percentage_max = 0.25f;
            bool royalty_noble_bonus = true;
            u16 royalty_noble_renown_min = 1;
            u16 royalty_noble_renown_max = 50;
        } lineage;
        
        struct AccomplishmentsConfig {
            u16 discovery_min = 25;
            u16 discovery_max = 150;
            u16 founding_min = 50;
            u16 founding_max = 300;
            u16 conflict_resolution_min = 100;
            u16 conflict_resolution_max = 500;
            u16 legendary_deed_min = 200;
            u16 legendary_deed_max = 1000;
        } accomplishments;
    } renown_sources;
    
    struct InfluenceEffectsConfig {
        f32 combat_security_min = 0.05f;
        f32 combat_security_max = 0.20f;
        f32 crafting_economy_min = 0.05f;
        f32 crafting_economy_max = 0.15f;
        f32 knowledge_education_min = 0.05f;
        f32 knowledge_education_max = 0.15f;
        f32 social_stability_min = 0.05f;
        f32 social_stability_max = 0.15f;
    } influence_effects;
};

// Race configuration
struct RaceDefinition {
    RaceID id = INVALID_RACE_ID;
    std::string name;
    f32 base_population_percentage = 0.0f;
    f32 lifespan_multiplier = 1.0f;
    u16 max_age = 80;
    f32 skill_progression_multiplier = 1.0f;
    std::vector<std::string> preferred_regions;
    std::vector<f32> preferred_region_weights;
    std::vector<std::string> avoided_regions;
    f32 migration_tendency = 0.5f;
    f32 fertility_rate = 1.0f;
    std::unordered_map<std::string, f32> skill_affinities;
    std::unordered_map<std::string, f32> skill_penalties;
};

struct RacesConfig {
    bool enabled = true;
    std::vector<RaceDefinition> races;
    
    struct InterracialBreedingConfig {
        bool enabled = true;
        f32 hybrid_race_probability = 0.3f;
        f32 inherit_race_probability = 0.7f;
    } interracial_breeding;
};

// Regions configuration
struct RegionsConfig {
    std::vector<std::string> types;
    u32 default_capacity = 10000;
    f32 capacity_variance = 0.5f;
    std::vector<std::string> resource_types;
    u8 trait_categories = 5;
    u8 neighbor_connections_min = 2;
    u8 neighbor_connections_max = 6;
    bool race_attraction_enabled = true;
    
    // Region definitions (loaded from JSON)
    std::unordered_map<std::string, Simulation::RegionDefinition> region_definitions;
    std::string region_data_path = "assets/data/regions.json";
};

// Events configuration
struct EventsConfig {
    u32 max_active_events = 100;
    u32 event_queue_size = 1000;
    f32 global_event_frequency = 0.001f;
    f32 regional_event_frequency = 0.01f;
    f32 individual_event_frequency = 0.1f;
    u32 event_history_size = 10000;
    f32 cascade_probability = 0.1f;
};

// Memory configuration
struct MemoryConfig {
    u32 target_memory_per_entity_bytes = 200;
    u32 max_memory_mb = 4096;
    u32 entity_pool_size = 0;  // 0 = auto-calculate
    f32 entity_pool_growth_factor = 1.5f;
    bool enable_memory_tracking = false;
    f32 memory_warning_threshold = 0.9f;
};

// Rendering configuration
struct RenderingConfig {
    bool enable_visualization = true;
    std::string entity_visualization = "representative";
    std::string region_visualization = "full";
    u32 update_visuals_every_n_ticks = 1;
    bool culling_enabled = true;
    bool lod_visual_detail = true;
};

// Main configuration class
class Configuration {
public:
    static Configuration& GetInstance();
    
    // Load configuration from file
    bool LoadFromFile(const std::string& filepath);
    bool LoadFromJSON(const std::string& jsonString);
    
    // Save configuration to file
    bool SaveToFile(const std::string& filepath) const;
    std::string SaveToJSON() const;
    
    // Validate configuration
    bool Validate() const;
    
    // Hot reload (development only)
    bool Reload();
    
    // Configuration sections
    WorldConfig world;
    PerformanceConfig performance;
    SimulationConfig simulation;
    SkillsConfig skills;
    HeroesConfig heroes;
    RacesConfig races;
    RegionsConfig regions;
    EventsConfig events;
    MemoryConfig memory;
    RenderingConfig rendering;
    
private:
    Configuration();
    ~Configuration() = default;
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;
    
    bool LoadDefaults();
    void ApplyDefaults();
};

} // namespace Config

