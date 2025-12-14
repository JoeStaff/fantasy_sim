# Anime Fantasy World/Life Simulator - Design Document

## Table of Contents
1. [Vision & Goals](#vision--goals)
2. [Core Requirements](#core-requirements)
3. [Configuration System](#configuration-system)
4. [System Architecture](#system-architecture)
5. [Simulation Levels (LOD System)](#simulation-levels-lod-system)
6. [Data Structures & Optimization](#data-structures--optimization)
7. [Development Milestones](#development-milestones)
8. [Design Principles & Rules](#design-principles--rules)
9. [Regional System](#regional-system)
10. [Race System](#race-system)
11. [Skill System](#skill-system)
12. [Hero System](#hero-system)
13. [Event System](#event-system)
14. [Technical Specifications](#technical-specifications)

---

## Vision & Goals

### Core Vision
Create an anime fantasy-themed world/life simulation game featuring a living, breathing world with 1 million inhabitants across 100 regions, where each individual has rich depth through 200 skills, regional traits, influences, and dynamic events.

### Ultimate Goals
- **Configurable population** (default: 1,000,000 living inhabitants) simultaneously simulated
- **Configurable skill count** (default: 200 distinct skills) per individual
- **Configurable region count** (default: 100 regions) forming a cohesive country/world
- **Hero system** with renown-based promotion and regional influence (fully configurable)
- **Multi-tier simulation** based on focus (full/half/formula-based) with configurable LOD settings
- **Real-time performance** on modern hardware (configurable target FPS/frame time)
- **Emergent gameplay** through interactions between systems

---

## Core Requirements

### Scale Requirements (All Configurable)
- Maximum population: Configurable (default: 1,000,000 inhabitants) - see `world.max_population`
- Regions: Configurable (default: 100) - see `world.region_count`
- Skills per individual: Configurable (default: 200) - see `skills.skill_count`
- Target performance: Configurable (default: 60 FPS) - see `performance.target_fps`
- Memory budget: Configurable target per entity - see `memory.target_memory_per_entity_bytes`

### Functional Requirements
1. **Individual Simulation**: Each inhabitant must have:
   - Race (species) with unique characteristics
   - 200 skills with levels/progression (mortal max: level 9, divine levels 10-15 disabled by default)
   - Regional traits (genetic/cultural)
   - Personal influences (relationships, status, etc.)
   - Life cycle (birth, aging, death) - race-dependent lifespan
   - Decision-making and goal-oriented behavior (race-influenced)
   - Hero status and renown (for special entities)

2. **Regional Simulation**: Each region must have:
   - Population management
   - Resource systems
   - Cultural traits
   - Political/influence systems
   - Environmental conditions

3. **Dynamic Events**: System must support:
   - Global events affecting multiple regions
   - Regional events
   - Individual events
   - Cascading effects

---

## Configuration System

### Overview

**Core Principle**: No hard-coded values. All system parameters must be configurable via settings files, runtime configuration, or user preferences. This enables:
- Easy tuning and balancing
- Performance scaling for different hardware
- Custom gameplay experiences
- Testing and debugging flexibility
- Future expansion without code changes

### Configuration Storage

**Primary Format**: JSON or TOML configuration files
- Human-readable and editable
- Supports comments for documentation
- Easy to version control
- Can be hot-reloaded (optional, for development)

**Configuration Hierarchy**:
1. **Default Config**: Built-in defaults (fallback values)
2. **User Config**: User-editable settings file
3. **Runtime Overrides**: Command-line arguments, UI settings
4. **Save Game Config**: Per-save-game configuration (for different world settings)

**Configuration Categories**:

### 1. World Settings

```json
{
  "world": {
    "max_population": 1000000,
    "initial_population": 10000,
    "region_count": 100,
    "region_size": 1000.0,
    "world_width": 10000.0,
    "world_height": 10000.0,
    "time_scale": 1.0,
    "tick_rate": 60.0
  }
}
```

**Parameters**:
- `max_population`: Maximum number of entities (u32)
- `initial_population`: Starting population (u32)
- `region_count`: Total number of regions (u8, max 255)
- `region_size`: Size of each region in world units (f32)
- `world_width/height`: Total world dimensions (f32)
- `time_scale`: Simulation speed multiplier (f32, 0.1 to 10.0)
- `tick_rate`: Target simulation ticks per second (f32)

### 2. Performance Settings

```json
{
  "performance": {
    "target_fps": 60.0,
    "target_frame_time_ms": 16.67,
    "thread_count": 0,
    "thread_count_override": false,
    "batch_size": 128,
    "simd_enabled": true,
    "parallel_processing": true,
    "cache_line_size": 64,
    "memory_pool_size_mb": 2048,
    "enable_profiling": false
  }
}
```

**Parameters**:
- `target_fps`: Target frames per second (f32)
- `target_frame_time_ms`: Target frame time in milliseconds (f32)
- `thread_count`: Number of worker threads (0 = auto-detect CPU cores)
- `thread_count_override`: If false, auto-detect; if true, use thread_count
- `batch_size`: Entity processing batch size (u32, typically 64, 128, or 256)
- `simd_enabled`: Enable SIMD optimizations (bool)
- `parallel_processing`: Enable multi-threaded processing (bool)
- `cache_line_size`: CPU cache line size in bytes (u32, typically 64)
- `memory_pool_size_mb`: Pre-allocated memory pool size (u32)
- `enable_profiling`: Enable performance profiling (bool)

### 3. Simulation Settings

```json
{
  "simulation": {
    "lod": {
      "focus_region_count": 3,
      "visible_region_count": 10,
      "full_sim_update_frequency": 1,
      "half_sim_update_frequency": 3,
      "formula_sim_update_frequency": 30,
      "lod_transition_smoothness": 0.5,
      "auto_focus_enabled": true
    },
    "entity": {
      "max_age": 120,
      "birth_rate_base": 0.02,
      "death_rate_base": 0.01,
      "aging_rate": 1.0,
      "enable_aging": true
    },
    "region": {
      "max_capacity_multiplier": 1.5,
      "resource_regeneration_rate": 0.1,
      "migration_enabled": true,
      "migration_rate": 0.001
    }
  }
}
```

**LOD Parameters**:
- `focus_region_count`: Maximum regions in full simulation (u8)
- `visible_region_count`: Maximum regions in half simulation (u8)
- `full_sim_update_frequency`: Update every N ticks (u32, typically 1)
- `half_sim_update_frequency`: Update every N ticks (u32, typically 2-4)
- `formula_sim_update_frequency`: Update every N ticks (u32, typically 10-60)
- `lod_transition_smoothness`: Smoothness factor for transitions (f32, 0.0-1.0)
- `auto_focus_enabled`: Automatically focus camera-centered regions (bool)

**Entity Parameters**:
- `max_age`: Maximum entity age in years (u16)
- `birth_rate_base`: Base birth rate per entity per year (f32)
- `death_rate_base`: Base death rate per entity per year (f32)
- `aging_rate`: Aging speed multiplier (f32)
- `enable_aging`: Enable aging system (bool)

**Region Parameters**:
- `max_capacity_multiplier`: How much population can exceed capacity (f32)
- `resource_regeneration_rate`: Resource regen per tick (f32)
- `migration_enabled`: Enable entity migration (bool)
- `migration_rate`: Migration probability per entity per tick (f32)

### 4. Skill System Settings

```json
{
  "skills": {
    "skill_count": 200,
    "max_skill_level": 15,
    "min_skill_level": 0,
    "mortal_max_level": 9,
    "divine_levels_enabled": false,
    "divine_level_min": 10,
    "divine_level_max": 15,
    "progression": {
      "base_probability_level_0": 0.1,
      "base_probability_level_5": 0.01,
      "base_probability_level_8": 0.001,
      "base_probability_level_9": 0.0001,
      "base_probability_level_10": 0.00001,
      "base_probability_level_14": 0.000001,
      "progression_curve_exponent": 2.0,
      "activity_multiplier_active": 10.0,
      "activity_multiplier_related": 2.0,
      "activity_multiplier_inactive": 0.1,
      "age_modifier_childhood": 1.5,
      "age_modifier_adolescence": 1.2,
      "age_modifier_prime": 1.0,
      "age_modifier_middle_age": 0.8,
      "age_modifier_elder": 0.9,
      "enable_skill_decay": false,
      "decay_probability": 0.0001
    },
    "hero_promotion": {
      "skill_milestone_level": 6,
      "renown_per_level_9": 10,
      "renown_per_additional_level_9": 5,
      "renown_per_divine_level": 50,
      "top_percentile_threshold": 0.001,
      "renown_top_percentile_min": 25,
      "renown_top_percentile_max": 100
    }
  }
}
```

**Parameters**:
- `skill_count`: Total number of skills per entity (u16)
- `max_skill_level`: Maximum skill level (u8, must be ≤ 15 for 4-bit storage)
- `min_skill_level`: Minimum skill level (u8, typically 0)
- Progression curve values for different skill levels
- Activity multipliers for skill usage
- Age modifiers for different life stages
- Hero promotion thresholds

### 5. Hero System Settings

```json
{
  "heroes": {
    "max_heroes": 1000,
    "hero_percentage": 0.001,
    "renown": {
      "min_renown": 1,
      "max_renown": 65535,
      "local_hero_threshold": 100,
      "regional_hero_threshold": 1000,
      "national_hero_threshold": 10000,
      "legendary_hero_threshold": 10001,
      "decay_enabled": false,
      "decay_rate_per_year": 1.0,
      "decay_inactivity_years": 5.0
    },
    "influence": {
      "local_hero_radius": 2,
      "regional_hero_radius": 5,
      "national_hero_radius": 10,
      "legendary_hero_radius": 20,
      "influence_update_frequency": 10,
      "distance_modifier_exponent": 1.0,
      "base_influence_divisor": 1000.0
    },
    "renown_sources": {
      "combat": {
        "monster_defeat_min": 50,
        "monster_defeat_max": 500,
        "criminal_defeat_min": 25,
        "criminal_defeat_max": 200,
        "battle_win_min": 100,
        "battle_win_max": 1000,
        "region_protection_min": 10,
        "region_protection_max": 100
      },
      "skill": {
        "level_15_renown": 10,
        "additional_level_15_renown": 5,
        "top_percentile_renown_min": 25,
        "top_percentile_renown_max": 100
      },
      "lineage": {
        "child_of_hero_min": 5,
        "child_of_hero_max": 50,
        "descendant_legendary_min": 10,
        "descendant_legendary_max": 100,
        "inheritance_percentage_min": 0.10,
        "inheritance_percentage_max": 0.25,
        "royalty_noble_bonus": true,
        "royalty_noble_renown_min": 1,
        "royalty_noble_renown_max": 50
      },
      "accomplishments": {
        "discovery_min": 25,
        "discovery_max": 150,
        "founding_min": 50,
        "founding_max": 300,
        "conflict_resolution_min": 100,
        "conflict_resolution_max": 500,
        "legendary_deed_min": 200,
        "legendary_deed_max": 1000
      }
    },
    "influence_effects": {
      "combat_security_min": 0.05,
      "combat_security_max": 0.20,
      "crafting_economy_min": 0.05,
      "crafting_economy_max": 0.15,
      "knowledge_education_min": 0.05,
      "knowledge_education_max": 0.15,
      "social_stability_min": 0.05,
      "social_stability_max": 0.15
    }
  }
}
```

**Parameters**:
- `max_heroes`: Maximum number of heroes (u32)
- `hero_percentage`: Expected percentage of population that are heroes (f32)
- Renown thresholds for different hero tiers
- Influence radius for each hero tier
- Renown gain amounts for different accomplishments
- Influence effect multipliers

### 6. Race System Settings

```json
{
  "races": {
    "enabled": true,
    "races": [
      {
        "id": "human",
        "name": "Human",
        "base_population_percentage": 0.60,
        "lifespan_multiplier": 1.0,
        "max_age": 80,
        "skill_progression_multiplier": 1.0,
        "preferred_regions": ["Urban", "Rural", "Plains"],
        "preferred_region_weights": [1.0, 1.0, 0.8],
        "avoided_regions": [],
        "migration_tendency": 0.5,
        "fertility_rate": 1.0,
        "skill_affinities": {},
        "skill_penalties": {}
      },
      {
        "id": "elf",
        "name": "Elf",
        "base_population_percentage": 0.15,
        "lifespan_multiplier": 3.0,
        "max_age": 300,
        "skill_progression_multiplier": 0.6,
        "preferred_regions": ["Forest", "Mountain"],
        "preferred_region_weights": [1.5, 1.2],
        "avoided_regions": ["Urban", "Desert"],
        "migration_tendency": 0.2,
        "fertility_rate": 0.4,
        "skill_affinities": {
          "archery": 1.3,
          "nature_magic": 1.5,
          "crafting": 1.2
        },
        "skill_penalties": {
          "mining": 0.8,
          "urban_skills": 0.7
        }
      },
      {
        "id": "dwarf",
        "name": "Dwarf",
        "base_population_percentage": 0.10,
        "lifespan_multiplier": 1.5,
        "max_age": 150,
        "skill_progression_multiplier": 1.1,
        "preferred_regions": ["Mountain", "Urban"],
        "preferred_region_weights": [1.5, 1.0],
        "avoided_regions": ["Forest", "Desert"],
        "migration_tendency": 0.3,
        "fertility_rate": 0.8,
        "skill_affinities": {
          "mining": 1.5,
          "blacksmithing": 1.4,
          "crafting": 1.3
        },
        "skill_penalties": {
          "archery": 0.8,
          "nature_magic": 0.7
        }
      },
      {
        "id": "orc",
        "name": "Orc",
        "base_population_percentage": 0.10,
        "lifespan_multiplier": 0.8,
        "max_age": 50,
        "skill_progression_multiplier": 1.2,
        "preferred_regions": ["Plains", "Desert"],
        "preferred_region_weights": [1.2, 1.0],
        "avoided_regions": ["Forest", "Mountain"],
        "migration_tendency": 0.7,
        "fertility_rate": 1.5,
        "skill_affinities": {
          "combat": 1.4,
          "strength": 1.3,
          "endurance": 1.2
        },
        "skill_penalties": {
          "magic": 0.7,
          "crafting": 0.8
        }
      },
      {
        "id": "halfling",
        "name": "Halfling",
        "base_population_percentage": 0.05,
        "lifespan_multiplier": 1.2,
        "max_age": 100,
        "skill_progression_multiplier": 0.9,
        "preferred_regions": ["Rural", "Plains"],
        "preferred_region_weights": [1.3, 1.1],
        "avoided_regions": ["Urban", "Mountain"],
        "migration_tendency": 0.4,
        "fertility_rate": 1.2,
        "skill_affinities": {
          "cooking": 1.4,
          "agriculture": 1.3,
          "social": 1.2
        },
        "skill_penalties": {
          "combat": 0.8,
          "mining": 0.7
        }
      }
    ],
    "interracial_breeding": {
      "enabled": true,
      "hybrid_race_probability": 0.3,
      "inherit_race_probability": 0.7
    }
  }
}
```

**Parameters**:
- `enabled`: Enable race system (bool)
- `races`: Array of race definitions
  - `id`: Unique identifier (string)
  - `name`: Display name (string)
  - `base_population_percentage`: Initial population distribution (f32, should sum to ~1.0)
  - `lifespan_multiplier`: Multiplier for base aging rate (f32)
  - `max_age`: Maximum age in years (u16)
  - `skill_progression_multiplier`: Base skill progression rate modifier (f32)
  - `preferred_regions`: List of region types this race prefers (array of strings)
  - `preferred_region_weights`: Attraction weights for preferred regions (array of f32)
  - `avoided_regions`: Region types this race avoids (array of strings)
  - `migration_tendency`: Likelihood to migrate (f32, 0.0-1.0)
  - `fertility_rate`: Birth rate multiplier (f32)
  - `skill_affinities`: Map of skill IDs to progression multipliers (object, f32 values)
  - `skill_penalties`: Map of skill IDs to progression penalties (object, f32 values)

### 7. Regional Settings

```json
{
  "regions": {
    "types": ["Urban", "Rural", "Forest", "Mountain", "Coastal", "Desert", "Plains"],
    "default_capacity": 10000,
    "capacity_variance": 0.5,
    "resource_types": ["Food", "Materials", "Magic", "Trade"],
    "trait_categories": 5,
    "neighbor_connections_min": 2,
    "neighbor_connections_max": 6,
    "race_attraction_enabled": true
  }
}
```

### 8. Event System Settings

```json
{
  "events": {
    "max_active_events": 100,
    "event_queue_size": 1000,
    "global_event_frequency": 0.001,
    "regional_event_frequency": 0.01,
    "individual_event_frequency": 0.1,
    "event_history_size": 10000,
    "cascade_probability": 0.1
  }
}
```

### 9. Memory Settings

```json
{
  "memory": {
    "target_memory_per_entity_bytes": 200,
    "max_memory_mb": 4096,
    "entity_pool_size": 0,
    "entity_pool_growth_factor": 1.5,
    "enable_memory_tracking": false,
    "memory_warning_threshold": 0.9
  }
}
```

**Parameters**:
- `target_memory_per_entity_bytes`: Target memory per entity (u32)
- `max_memory_mb`: Maximum memory usage in MB (u32)
- `entity_pool_size`: Pre-allocated entity pool (0 = auto-calculate)
- `entity_pool_growth_factor`: Growth factor when pool expands (f32)
- `enable_memory_tracking`: Track memory usage (bool)
- `memory_warning_threshold`: Warn when memory usage exceeds this (f32, 0.0-1.0)

### 10. Rendering/Visual Settings

```json
{
  "rendering": {
    "enable_visualization": true,
    "entity_visualization": "representative",
    "region_visualization": "full",
    "update_visuals_every_n_ticks": 1,
    "culling_enabled": true,
    "lod_visual_detail": true
  }
}
```

### Configuration Access Pattern

**In Code**:
```cpp
// Access configuration values
auto& config = Configuration::GetInstance();
u32 max_pop = config.world.max_population;
f32 frame_time = config.performance.target_frame_time_ms;
u8 skill_count = config.skills.skill_count;

// Runtime modification (if allowed)
config.world.max_population = 2000000;  // If validation passes
```

**Validation Rules**:
- All configuration values must be validated on load
- Invalid values fall back to defaults
- Warnings logged for invalid values
- Critical values (like max_population) have hard limits to prevent crashes

**Hot Reloading** (Development Only):
- Watch configuration file for changes
- Reload and validate on change
- Apply new settings (if safe)
- Useful for tuning without restarting

### Configuration File Locations

1. **Default Config**: `config/default.json` (bundled with game)
2. **User Config**: `config/user.json` (user-editable, overrides default)
3. **Save Game Config**: `saves/<save_name>/config.json` (per-save settings)
4. **Command Line**: `--config <path>` (override all)

### Design Rules for Configuration

1. **No Magic Numbers**: Every numeric constant must reference configuration
2. **Sensible Defaults**: Default values should work well for target hardware
3. **Documentation**: All configuration parameters must be documented
4. **Validation**: All values must be validated (ranges, types, dependencies)
5. **Backwards Compatibility**: Old config files should still work (with migrations)
6. **Performance Impact**: Configuration lookups should be fast (cached if needed)

---

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Simulation Manager                       │
│              (Orchestrates all simulation layers)            │
└──────────────────┬──────────────────────────────────────────┘
                   │
       ┌───────────┼───────────┐
       │           │           │
┌──────▼──────┐ ┌──▼──────┐ ┌─▼──────────┐
│ Full Sim    │ │ Half Sim│ │Formula Sim │
│ (Focus)     │ │ (Visible)│ │ (Offscreen)│
└──────┬──────┘ └──┬──────┘ └─┬──────────┘
       │           │           │
┌──────▼───────────▼───────────▼──────────────┐
│         Entity Component System (ECS)        │
│  (Inhabitants, Regions, Events, Resources)   │
└──────────────────────────────────────────────┘
```

### Core Systems

1. **Entity Component System (ECS)**
   - Entities: Unique IDs (64-bit integers)
   - Components: Data-only structures (SoA - Structure of Arrays preferred)
   - Systems: Processors that operate on components

2. **Simulation Manager**
   - Manages LOD transitions
   - Coordinates update cycles
   - Handles region focus changes

3. **Spatial Partitioning**
   - Region-based partitioning
   - Quadtree/Octree for sub-region spatial queries
   - Hierarchical culling

4. **Data Manager**
   - Entity storage (SoA vs AoS tradeoffs)
   - Memory pools for allocation
   - Serialization/deserialization

5. **Event System**
   - Priority queues for event scheduling
   - Event propagation system
   - Event history/undo (optional)

---

## Simulation Levels (LOD System)

### Level 0: Full Simulation (Focus Regions)
**Activation**: User-selected or camera-focused regions (configurable: `simulation.lod.focus_region_count`, default: 1-3)

**Update Frequency**: Every N ticks (configurable: `simulation.lod.full_sim_update_frequency`, default: 1)

**Features**:
- Individual entity updates (movement, decisions, interactions)
- Real-time probabilistic skill progression (all skills calculated per entity, count: `skills.skill_count`)
- Full relationship networks
- Detailed pathfinding
- Complete event processing
- Visual representation updates
- **Hero entities**: Always in full simulation regardless of region focus

**Performance Target**: Configurable (default: <16.67ms per frame) - see `performance.target_frame_time_ms`

### Level 1: Half Simulation (Visible Regions)
**Activation**: Regions visible on screen but not in direct focus (configurable: `simulation.lod.visible_region_count`, default: 5-10)

**Update Frequency**: Every N ticks (configurable: `simulation.lod.half_sim_update_frequency`, default: 2-4)

**Features**:
- Simplified entity updates (batched processing)
- Reduced-frequency skill progression (calculate every N ticks or for subset of skills)
- Representative sampling for skill progression calculations
- Sparse relationship updates
- Simplified pathfinding (grid-based)
- Event processing with reduced detail
- Visual updates (lower frequency)

**Performance Target**: Configurable (default: <8ms per frame) - proportionally scaled from `performance.target_frame_time_ms`

### Level 2: Formula-Based Simulation (Offscreen Regions)
**Activation**: All regions not in focus or visible (remaining regions after focus + visible)

**Update Frequency**: Every N ticks (configurable: `simulation.lod.formula_sim_update_frequency`, default: 10-60, can vary by distance)

**Features**:
- Statistical population models
- Aggregate skill distributions
- Formula-based population changes
- Event probability calculations
- No individual entity tracking
- Periodic sync with full simulation data
- **Hero entities**: Always tracked individually even in offscreen regions (maintain simulation priority)

**Performance Target**: Configurable (default: <2ms per frame) - proportionally scaled from `performance.target_frame_time_ms`

### LOD Transition Rules

1. **Focus → Half**: When region leaves focus
   - Convert individuals to statistical representation
   - Store key state snapshots
   - Reduce update frequency

2. **Half → Formula**: When region leaves visible area
   - Aggregate individual data
   - Create statistical models
   - Remove individual entities

3. **Formula → Half**: When region enters visible area
   - Generate representative population from statistics
   - Initialize with approximate skill distributions
   - Begin half-simulation

4. **Half → Focus**: When region enters focus
   - Generate full population from statistics/representatives
   - Initialize relationships
   - Begin full simulation

### Hero Simulation Priority

**Heroes maintain full simulation regardless of region LOD**:
- Heroes are always tracked individually
- Full skill progression calculations
- Complete relationship networks
- Detailed movement and decision-making
- Regional influence calculations
- **Rationale**: Heroes are rare (<0.1% of population) and have significant impact
- **Performance Impact**: Minimal (typically <1000 heroes total)

---

## Data Structures & Optimization

### Entity Storage Strategy: Structure of Arrays (SoA)

**Priority**: Performance > Memory > Readability

```
Traditional (AoS - slower for bulk operations):
[Entity{id, skill1, skill2, ...}, Entity{id, skill1, skill2, ...}]

Optimized (SoA - faster for bulk operations):
Skills: [skill1_array, skill2_array, ..., skill200_array]
IDs: [id_array]
Position: [x_array, y_array]
```

### Core Data Structures

1. **Inhabitant Components** (SoA)
   - `id`: u64 array
   - `skills`: u4[N] array (N = `skills.skill_count`, 4 bits each = N/2 bytes per entity)
     - Maximum skill level: `skills.max_skill_level` (must be ≤ 15 for 4-bit storage)
     - Mortal maximum: `skills.mortal_max_level` (default: 9, levels 0-9 achievable by mortals)
     - Divine levels: `skills.divine_level_min` to `skills.divine_level_max` (default: 10-15, disabled by default)
     - Packed storage: 2 skills per byte, or byte array with bit manipulation
   - `race_id`: u8 array (race identifier, max 255 races)
   - `region_id`: u8 array (max regions = `world.region_count`, must be ≤ 255)
   - `age`: u16 array (max age = race-specific `races.races[race_id].max_age`)
   - `traits`: bitfield array (compressed traits)
   - `influences`: sparse array or separate influence table
   - `is_hero`: bool array (1 bit per entity, can be part of traits bitfield)
   - `renown`: u16 array (only for heroes, sparse storage for regular entities)
     - Range: `heroes.renown.min_renown` to `heroes.renown.max_renown` (default: 0-65535)
     - Regular entities: renown = 0 (implicit, not stored)
     - Heroes: renown value determines influence and status

2. **Regional Data**
   - `population_count`: u32
   - `skill_distributions`: f32[N] where N = `skills.skill_count` (mean/std dev per skill)
   - `resources`: various typed arrays (types from `regions.resource_types`)
   - `traits`: bitfield

3. **Spatial Indexing**
   - Region grid (size = `world.region_count`)
   - Sub-region quadtrees for spatial queries
   - Entity-to-region mapping (hash table)
   - Hero registry: Separate list/array of hero entity IDs
     - Fast lookup for hero-specific operations
     - Maintained separately for performance
     - Maximum size: `heroes.max_heroes` (default: <1000, ~0.1% of population)

### Optimization Techniques

1. **Memory Pool Allocation**
   - Pre-allocate large blocks
   - Use object pools for temporary objects
   - Minimize allocations during simulation

2. **Batch Processing**
   - Process entities in chunks (configurable: `performance.batch_size`, default: 64, 128, or 256)
   - Use SIMD instructions where possible (configurable: `performance.simd_enabled`)
   - Parallel processing for independent operations (configurable: `performance.parallel_processing`, thread count: `performance.thread_count`)

3. **Data Locality**
   - Keep related data close in memory
   - Separate hot/cold data paths
   - Use bitfields for flags/booleans

4. **Sparse Data Handling**
   - Use sparse arrays/maps for optional data
   - Compress inactive entity data
   - Lazy evaluation for expensive operations

5. **Caching Strategies**
   - Cache frequently accessed lookups
   - Precompute common calculations
   - Use spatial hashing for proximity queries

---

## Development Milestones

### Phase 0: Foundation (MVP - Minimal Viable Prototype)
**Goal**: Prove core architecture works with small scale

- [ ] Configuration system implementation
- [ ] Basic ECS implementation
- [ ] Single region with configurable inhabitants (default: 100)
- [ ] Configurable skills per individual (default: 10)
- [ ] Basic LOD system (full/half/formula) with configurable settings
- [ ] Simple rendering/viewport system
- [ ] Performance profiling framework

**Success Criteria**: 
- Can simulate 100 entities at 60 FPS
- LOD transitions work smoothly
- Foundation is extensible

### Phase 1: Core Systems
**Goal**: Expand to multiple regions with core simulation

- [ ] Configuration system fully integrated (all systems use config)
- [ ] Configurable regions (default: 10) with full/half/formula simulation
- [ ] Configurable skills per individual (default: 50)
- [ ] Basic regional traits system (configurable types)
- [ ] Population management (birth/death) with configurable rates
- [ ] Skill progression system (all probabilities configurable)
- [ ] Simple event system (configurable frequencies)

**Success Criteria**:
- Can simulate 10,000 entities across 10 regions at 60 FPS
- Regional transitions are seamless
- Core gameplay loop is fun

### Phase 2: Scale Expansion
**Goal**: Reach 10% of target scale

- [ ] 100 regions (all regions implemented)
- [ ] 100 skills per individual
- [ ] 100,000 inhabitants (10% of target)
- [ ] Advanced LOD system with smooth transitions
- [ ] Regional influence system
- [ ] Complex event system
- [ ] Basic hero system (renown tracking, promotion)
- [ ] Hero simulation priority (always full sim)

**Success Criteria**:
- Can simulate 100,000 entities at reasonable frame rates
- All LOD levels working correctly
- Memory usage is manageable
- Heroes maintain full simulation regardless of region LOD

### Phase 3: Full Feature Set
**Goal**: Complete all planned features

- [ ] 200 skills per individual
- [ ] 500,000 inhabitants
- [ ] Complete trait system
- [ ] Full event system with cascading effects
- [ ] Relationship/influence networks
- [ ] Advanced AI/decision-making
- [ ] Complete hero system (renown sources, regional influence)
- [ ] Hero events and lineage system
- [ ] Hero influence calculations and effects

**Success Criteria**: 
- All features implemented
- Performance is acceptable at 500k scale
- Gameplay is engaging
- Heroes properly influence regions and trigger events

### Phase 4: Optimization & Polish
**Goal**: Reach final scale and optimize

- [ ] 1,000,000 inhabitants
- [ ] Performance optimization pass
- [ ] Memory optimization
- [ ] Save/load system
- [ ] UI/UX polish
- [ ] Balancing and tuning

**Success Criteria**:
- 1M entities running at target performance
- Stable and playable experience
- Ready for release

---

## Design Principles & Rules

### Performance Rules

1. **Never allocate memory during main simulation loop**
   - Pre-allocate all pools
   - Reuse temporary objects

2. **Batch process everything**
   - Process entities in cache-friendly chunks
   - Minimize random memory access

3. **Profile before optimizing**
   - Identify bottlenecks with actual data
   - Optimize hot paths first

4. **Favor data-oriented design over object-oriented**
   - SoA over AoS
   - Systems over classes
   - Data transforms over stateful objects

5. **LOD is mandatory, not optional**
   - Every system must support multiple detail levels
   - No "one size fits all" solutions

### Architecture Rules

1. **ECS is the foundation**
   - Everything is an entity or component
   - Systems process components, not entities directly
   - No direct entity-to-entity references (use IDs)

2. **Immutable data where possible**
   - Components are data, systems transform them
   - Prefer functional transformations
   - Easier to parallelize

3. **Region-centric design**
   - Regions are the primary organizational unit
   - Entities belong to regions
   - Systems operate per-region

4. **Event-driven for non-critical paths**
   - Use events for notifications
   - Don't poll, use callbacks/events
   - Priority queues for scheduled events

5. **Explicit over implicit**
   - Clear ownership of data
   - Explicit state transitions
   - **No magic numbers (use configuration values)**
   - All numeric constants must reference configuration system

### Code Quality Rules

1. **Readability > Premature optimization**
   - Write clear code first
   - Profile, then optimize
   - Document performance-critical sections

2. **Test at scale early**
   - Don't optimize for 100 entities if target is 1M
   - Use realistic data in tests
   - Load test regularly

3. **Version control is essential**
   - Tag performance milestones
   - Keep optimization benchmarks
   - Track performance regressions

4. **Modularity and pluggability**
   - Systems should be independently testable
   - Easy to enable/disable features
   - Clear interfaces between systems

5. **Configuration-driven design**
   - No hard-coded values (use configuration system)
   - All numeric constants must reference configuration
   - Sensible defaults for all settings
   - Easy to tune without code changes

---

## Regional System

### Region Structure

**Region Properties**:
- ID: 0-99 (u8)
- Type: Urban, Rural, Forest, Mountain, Coastal, etc.
- Population: Current count (can exceed capacity temporarily)
- Capacity: Maximum sustainable population
- Resources: Food, Materials, Magic, etc.
- Traits: Cultural, Environmental, Political
- Neighbors: Connected region IDs
- Position: World coordinates
- **Hero Influences**: List of heroes affecting this region
  - Hero IDs and influence strengths
  - Cached for performance
  - Updated when heroes move or renown changes

### Regional Traits

**Categories**:
1. **Environmental**: Climate, terrain, resources
2. **Cultural**: Traditions, values, customs
3. **Political**: Government type, stability
4. **Economic**: Wealth level, trade routes
5. **Magical**: Mana density, magical phenomena

**Implementation**: Bitfield or enum-based flags with modifiers

### Regional Simulation

**Full Simulation**:
- Individual entity tracking
- Resource consumption per entity
- Detailed interactions
- Hero influence effects (if heroes present)
- Race-based regional preferences affecting settlement and migration

**Half Simulation**:
- Aggregated resource consumption
- Representative entity sampling
- Simplified interactions

**Formula Simulation**:
- Statistical resource models
- Population growth formulas (race-specific fertility rates)
- Event probability calculations
- Race distribution models (preferred regions attract certain races)

---

## Race System

### Overview

The Race System introduces different species/races with unique characteristics that affect skill progression, lifespan, regional preferences, and behavior. This adds depth and diversity to the simulation while maintaining performance efficiency.

### Core Race Mechanics

**Race Properties**:
- **Lifespan**: Different races have different maximum ages and aging rates
- **Skill Progression**: Races learn skills at different rates
- **Regional Preferences**: Races prefer certain region types and avoid others
- **Skill Affinities**: Races have bonuses/penalties for specific skills
- **Behavioral Traits**: Migration tendencies, fertility rates, etc.

### Race Data Structure

**Per-Race Configuration** (from `races.races[]`):
- `id`: Unique identifier (u8, max 255 races)
- `name`: Display name
- `base_population_percentage`: Initial distribution (f32)
- `lifespan_multiplier`: Aging rate modifier (f32)
- `max_age`: Maximum age in years (u16)
- `skill_progression_multiplier`: Base skill learning rate (f32)
- `preferred_regions`: Array of region type IDs (u8[])
- `preferred_region_weights`: Attraction weights (f32[])
- `avoided_regions`: Array of region type IDs to avoid (u8[])
- `migration_tendency`: Likelihood to migrate (f32, 0.0-1.0)
- `fertility_rate`: Birth rate multiplier (f32)
- `skill_affinities`: Map of skill_id → multiplier (f32)
- `skill_penalties`: Map of skill_id → penalty (f32)

### Race Effects on Simulation

#### 1. Aging and Lifespan

**Aging Rate**:
```
entity_aging_rate = base_aging_rate * races[race_id].lifespan_multiplier
```

**Maximum Age**:
```
max_age = races[race_id].max_age
```

**Examples**:
- Human: 1.0x aging, max 80 years
- Elf: 0.33x aging (3x slower), max 300 years
- Dwarf: 0.67x aging (1.5x slower), max 150 years
- Orc: 1.25x aging (0.8x faster), max 50 years

#### 2. Skill Progression

**Race Modifier Applied**:
```
base_probability = skill_progression_curve(current_level)
race_multiplier = races[race_id].skill_progression_multiplier
skill_affinity = races[race_id].skill_affinities[skill_id] or 1.0
skill_penalty = races[race_id].skill_penalties[skill_id] or 1.0

final_probability = base_probability * race_multiplier * skill_affinity * skill_penalty * other_modifiers
```

**Examples**:
- Elves: 0.6x base progression (learn slower), but +1.5x for nature magic
- Dwarves: 1.1x base progression, +1.5x for mining/blacksmithing
- Orcs: 1.2x base progression, +1.4x for combat, but 0.7x for magic

#### 3. Regional Preferences

**Migration Attraction**:
```
attraction_score = base_attraction
if region_type in races[race_id].preferred_regions:
    weight = races[race_id].preferred_region_weights[index]
    attraction_score *= weight
if region_type in races[race_id].avoided_regions:
    attraction_score *= 0.1  // Strong avoidance
```

**Settlement Probability**:
- Races more likely to settle in preferred regions
- Races avoid settling in avoided regions
- Affects initial population distribution
- Influences migration patterns

**Examples**:
- Elves: Prefer Forest (1.5x), Mountain (1.2x); Avoid Urban (0.1x), Desert (0.1x)
- Dwarves: Prefer Mountain (1.5x), Urban (1.0x); Avoid Forest (0.1x), Desert (0.1x)
- Humans: Prefer Urban (1.0x), Rural (1.0x), Plains (0.8x); No strong aversions

#### 4. Fertility and Population

**Birth Rate**:
```
birth_rate = base_birth_rate * races[race_id].fertility_rate
```

**Population Distribution**:
- Initial distribution based on `base_population_percentage`
- Evolves over time based on fertility rates and migration
- Some races may become more/less common over time

**Examples**:
- Elves: 0.4x fertility (slow population growth)
- Orcs: 1.5x fertility (fast population growth)
- Humans: 1.0x fertility (baseline)

#### 5. Migration Behavior

**Migration Tendency**:
```
migration_probability = base_migration_rate * races[race_id].migration_tendency
```

**Migration Direction**:
- Influenced by regional preferences
- Races migrate toward preferred regions
- Races migrate away from avoided regions

**Examples**:
- Elves: 0.2x migration (settled, prefer to stay in forests)
- Orcs: 0.7x migration (nomadic, move frequently)
- Humans: 0.5x migration (moderate mobility)

### Race Storage

**Entity Component**:
- `race_id`: u8 (1 byte per entity)
- Lookup table: `races[race_id]` → Race configuration
- Total overhead: 1 byte per entity (negligible)

**Race Registry**:
- Separate data structure for race definitions
- Loaded from configuration
- Fast lookup via race_id
- Size: ~100-500 bytes per race definition (negligible for <50 races)

### Interracial Breeding

**Hybrid Races** (if `races.interracial_breeding.enabled`):
- When entities of different races have children
- Probability: `races.interracial_breeding.hybrid_race_probability` (default: 0.3)
- Hybrid inherits traits from both parents (averaged or blended)
- Otherwise inherits one parent's race (probability: `races.interracial_breeding.inherit_race_probability`)

**Hybrid Properties**:
- Lifespan: Average of parents
- Skill progression: Average of parents
- Regional preferences: Combined (prefers regions liked by either parent)
- Skill affinities: Blended from both parents

### Performance Considerations

**Overhead**:
- 1 byte per entity for race_id: ~1 MB for 1M entities (negligible)
- Race lookup: O(1) via array index
- Race modifiers: Simple multiplications (very fast)
- Regional preference calculations: Only during migration/settlement (infrequent)

**Optimization**:
- Pre-compute race modifiers for common operations
- Cache race-region attraction scores
- Batch process race-based calculations

### Example Race Configurations

**Elf** (from configuration):
- Lifespan: 300 years (3x human)
- Skill progression: 0.6x (learns slower)
- Prefers: Forest (1.5x), Mountain (1.2x)
- Avoids: Urban (0.1x), Desert (0.1x)
- Affinities: Archery (+1.3x), Nature Magic (+1.5x), Crafting (+1.2x)
- Penalties: Mining (0.8x), Urban Skills (0.7x)
- Migration: 0.2x (settled)
- Fertility: 0.4x (slow growth)

**Dwarf**:
- Lifespan: 150 years (1.5x human)
- Skill progression: 1.1x (learns faster)
- Prefers: Mountain (1.5x), Urban (1.0x)
- Avoids: Forest (0.1x), Desert (0.1x)
- Affinities: Mining (+1.5x), Blacksmithing (+1.4x), Crafting (+1.3x)
- Penalties: Archery (0.8x), Nature Magic (0.7x)
- Migration: 0.3x (somewhat settled)
- Fertility: 0.8x (moderate growth)

---

## Skill System

### Skill Categories (200 Total)

**Proposed Distribution**:
- Combat: 30 skills (swords, magic, archery, etc.)
- Crafting: 40 skills (blacksmithing, alchemy, cooking, etc.)
- Social: 30 skills (persuasion, leadership, intimidation, etc.)
- Knowledge: 40 skills (history, magic theory, geography, etc.)
- Physical: 30 skills (strength, agility, endurance, etc.)
- Specialized: 30 skills (region-specific, rare abilities)

### Skill Storage

**Storage Format**: 4-bit values (nibbles)
- **Range**: 0 to `skills.max_skill_level` (must be ≤ 15 for 4-bit storage)
- **Mortal Maximum**: `skills.mortal_max_level` (default: 9) - theoretical maximum for mortal beings
- **Divine Levels**: `skills.divine_level_min` to `skills.divine_level_max` (default: 10-15)
  - Only achievable if `skills.divine_levels_enabled` = true
  - In basic settings, divine levels are disabled (mortals capped at level 9)
  - Represents exponential growth and breaching divinity
- **Per Entity**: `skills.skill_count` skills × 0.5 bytes = **N/2 bytes** per entity (default: 100 bytes for 200 skills)
- **Total Memory**: `skills.skill_count` × 0.5 bytes × `world.max_population` (default: 100 MB for 1M entities)
- **Memory Savings**: vs u16 storage: **75% reduction** (300 bytes saved per entity at default settings)

**Storage Implementation Options**:

1. **Packed Byte Array** (Recommended):
   - 2 skills per byte (100 bytes per entity)
   - Access via bit manipulation (bit shifts and masks)
   - Implementation example:
     ```cpp
     // Storage: uint8_t skills[100];  // 200 skills packed
     
     // Get skill at index (0-199)
     uint8_t get_skill(uint8_t* skills, int skill_index) {
         int byte_index = skill_index / 2;
         bool is_low_nibble = (skill_index % 2) == 0;
         return is_low_nibble 
             ? (skills[byte_index] & 0x0F)        // Lower 4 bits
             : ((skills[byte_index] >> 4) & 0x0F); // Upper 4 bits
     }
     
     // Set skill at index
     void set_skill(uint8_t* skills, int skill_index, uint8_t level) {
         int byte_index = skill_index / 2;
         bool is_low_nibble = (skill_index % 2) == 0;
         if (is_low_nibble) {
             skills[byte_index] = (skills[byte_index] & 0xF0) | (level & 0x0F);
         } else {
             skills[byte_index] = (skills[byte_index] & 0x0F) | ((level & 0x0F) << 4);
         }
     }
     ```
   - Most memory-efficient
   - Slight CPU overhead for bit manipulation (negligible vs memory savings)

2. **SIMD Optimization** (Advanced):
   - Process multiple entities simultaneously
   - 16 entities' skills (16 × 200 = 3200 skills) in parallel operations
   - Use SIMD bit manipulation instructions (e.g., AVX-512)
   - Significant performance boost for bulk operations

3. **Hybrid Approach**:
   - Packed storage for memory
   - Unpack to temporary arrays for hot processing loops
   - Repack after updates
   - Good balance of memory and CPU performance

**Full Simulation**: Packed byte arrays with individual skill levels
```
skills[entity_id] = packed_byte_array[100]  // 200 skills packed into 100 bytes
// Access: get_skill(entity_id, skill_id) = extract_nibble(skills[entity_id], skill_id)
```

**Half Simulation**: Approximate distributions (still uses 4-bit for representatives)
```
region_skill_mean[region_id][skill_id] = f32 (mean level 0.0-15.0)
region_skill_std[region_id][skill_id] = f32 (standard deviation)
representative_entities[] = packed_skill_arrays (sampling of population)
```

**Formula Simulation**: Aggregate statistics only

---

## Hero System

### Overview

Heroes are special entities that maintain simulation priority and influence nearby regions. They represent exceptional individuals who have achieved renown through various accomplishments. Heroes are rare (estimated <0.1% of population, ~100-1000 heroes total) but have significant impact on the world.

### Hero Characteristics

**Core Properties**:
- **Always in Full Simulation**: Heroes maintain individual tracking regardless of region LOD
- **Renown Value**: Determines influence radius and magnitude
- **Regional Influence**: Affects nearby regions through their presence
- **Persistent Tracking**: Never converted to statistical representation
- **Extended Lifespan**: Heroes may have longer lifespans or special aging rules

**Estimated Scale**:
- Total Heroes: 100-1000 (0.01% to 0.1% of population)
- Active Heroes: 50-500 (heroes currently alive)
- Hero Density: ~1-10 heroes per region on average

### Renown System

**Renown Range**: Configurable (default: `heroes.renown.min_renown` to `heroes.renown.max_renown`, typically 1-65535)
- **0**: Not a hero (regular entity)
- **1 to `heroes.renown.local_hero_threshold`**: Local Hero (influences `heroes.influence.local_hero_radius` regions)
- **`heroes.renown.local_hero_threshold`+1 to `heroes.renown.regional_hero_threshold`**: Regional Hero (influences `heroes.influence.regional_hero_radius` regions)
- **`heroes.renown.regional_hero_threshold`+1 to `heroes.renown.national_hero_threshold`**: National Hero (influences `heroes.influence.national_hero_radius` regions)
- **`heroes.renown.national_hero_threshold`+1 to `heroes.renown.max_renown`**: Legendary Hero (influences `heroes.influence.legendary_hero_radius`+ regions, global impact)

**Renown Sources** (ways to gain renown):

1. **Combat Achievements**:
   - Defeating notorious monsters: +50 to +500 renown (based on monster threat level)
   - Defeating powerful criminals: +25 to +200 renown
   - Winning major battles/wars: +100 to +1000 renown
   - Protecting regions from threats: +10 to +100 renown per region saved

2. **Skill Mastery**:
   - Reaching skill level 15 in any skill: +10 renown
   - Reaching skill level 15 in multiple skills: +5 renown per additional skill
   - Being top 0.1% in a skill category: +25 to +100 renown
   - Creating masterwork items (requires high crafting skills): +50 to +200 renown

3. **Lineage**:
   - Being child of a hero: +5 to +50 renown (based on parent's renown)
   - Being descendant of legendary hero: +10 to +100 renown
   - Inheriting hero status: Starts with 10-25% of parent's renown
   - **Note**: Lineage alone rarely makes someone a hero, but provides head start, unless royalty or nobel

4. **Special Accomplishment Examples**:
   - Discovering new regions/resources: +25 to +150 renown
   - Founding settlements/organizations: +50 to +300 renown
   - Resolving major conflicts: +100 to +500 renown
   - Performing legendary deeds: +200 to +1000 renown (rare events)

**Renown Decay** (Optional):
- Heroes may lose renown if inactive for extended periods
- Very slow decay (e.g., -1 renown per year if no accomplishments)
- Prevents accumulation of too many high-renown heroes over time

### Hero Promotion System

**Promotion Threshold**: Renown ≥ 1
- Any entity with renown ≥ 1 is considered a hero
- Promotion happens automatically when renown crosses threshold
- No explicit "promotion event" needed (renown is the status)

**Promotion Triggers** (when renown is calculated/updated):

1. **Combat Resolution**:
   - After defeating notable enemy
   - System checks if renown gain crosses threshold
   - If yes, entity becomes hero (if not already)

2. **Skill Milestone**:
   - When skill reaches `skills.hero_promotion.skill_milestone_level` (default: 6)
   - System checks total renown
   - Promotes if threshold crossed
   - **Note**: Reaching `skills.mortal_max_level` (9) grants significant renown

3. **Event Completion**:
   - After major accomplishment events
   - Renown awarded, promotion checked

4. **Birth/Inheritance**:
   - Child of hero may start with renown
   - Child of royalty or nobel may start with renown
   - Promotion checked at birth if renown ≥ 1

**Promotion Effects**:
- Entity flagged as hero (`is_hero = true`)
- Added to hero registry for fast lookup
- Begins influencing nearby regions
- Maintains full simulation priority
- May trigger hero-related events

### Regional Influence System

**Influence Mechanics**:

Heroes influence regions based on:
- **Renown Level**: Higher renown = larger influence radius and stronger effects
- **Proximity**: Closer regions receive stronger influence
- **Hero Type**: Different heroes affect different aspects (combat, economy, culture, etc.)

**Influence Radius** (Configurable in `heroes.influence`):
```
Local Hero:     heroes.influence.local_hero_radius regions (default: 1-2)
Regional Hero:  heroes.influence.regional_hero_radius regions (default: 3-5)
National Hero:  heroes.influence.national_hero_radius regions (default: 6-10)
Legendary Hero: heroes.influence.legendary_hero_radius+ regions (default: 10+, up to global)
```

**Influence Calculation** (Configurable):
```
influence_strength = (renown / heroes.influence.base_influence_divisor) * distance_modifier
distance_modifier = 1.0 / (1.0 + distance_in_regions^heroes.influence.distance_modifier_exponent)
```

**Influence Effects** (applied to influenced regions):

1. **Combat Heroes**:
   - Increased security/stability: +5% to +20% (based on renown)
   - Reduced monster/criminal spawn rates
   - Improved military skill progression for inhabitants
   - Defense bonuses during conflicts

2. **Crafting Heroes**:
   - Economic boost: +5% to +15% resource production
   - Improved crafting skill progression
   - Higher quality items produced in region
   - Trade route bonuses

3. **Knowledge Heroes**:
   - Cultural/educational boost: +5% to +15% knowledge skill progression
   - Research/discovery bonuses
   - Improved regional traits over time
   - Innovation events more likely

4. **Social Heroes**:
   - Political stability: +5% to +15%
   - Improved social skill progression
   - Better relationship networks
   - Diplomatic bonuses

5. **Multi-Skill Heroes** (high renown in multiple categories):
   - Combined effects from multiple categories
   - Synergistic bonuses
   - Regional development acceleration

**Influence Updates**:
- Calculated every N ticks (configurable, e.g., every 10 ticks)
- Cached and updated when hero moves or renown changes
- Applied as modifiers to regional simulation formulas

### Hero Data Structures

**Hero-Specific Storage** (Sparse, only for heroes):

```cpp
struct HeroData {
    u64 entity_id;           // Reference to entity
    u16 renown;              // Current renown value
    u8 influence_radius;     // Number of regions influenced
    u8 hero_type;           // Combat, Crafting, Knowledge, Social, Mixed
    u16[] influenced_regions; // Array of region IDs (sparse)
    f32[] influence_strengths; // Influence strength per region
    u32 last_accomplishment_tick; // For decay calculations
    u32 promotion_tick;      // When they became a hero
};

// Storage: Separate array/map for heroes only
// Lookup: O(1) via entity_id hash map
// Size: ~50-100 bytes per hero (negligible for <1000 heroes)
```

**Integration with Entity System**:
- `is_hero` flag in entity traits bitfield (1 bit)
- `renown` in entity component (u16, sparse - only stored if > 0)
- Hero registry: Separate data structure for fast hero-only operations
- Cross-reference: Entity ID → Hero Data lookup

### Hero Lifecycle

**Birth**:
- Check if parents are heroes
- Calculate inherited renown (if any)
- If renown ≥ 1, promote to hero at birth
- Otherwise, regular entity (may become hero later)

**Life**:
- Heroes tracked individually at all times
- Full simulation regardless of region LOD
- Renown can increase through accomplishments
- Influence regions continuously
- May have extended lifespan (optional feature)

**Death**:
- Hero death is significant event
- May trigger regional events (mourning, succession, etc.)
- Children may inherit partial renown
- Legacy effects may persist (reduced influence for N years)
- Hero removed from registry

### Performance Considerations

**Hero Overhead**:
- **Memory**: ~50-100 bytes per hero × 1000 heroes = 50-100 KB (negligible)
- **CPU**: Full simulation for <1000 entities (vs 1M regular entities)
- **Impact**: <1% of total simulation cost

**Optimization Strategies**:
1. **Separate Hero Processing**: Process heroes in dedicated system
2. **Influence Caching**: Cache influence calculations, update only when needed
3. **Spatial Indexing**: Fast lookup of heroes near regions
4. **Batch Updates**: Update hero influences in batches
5. **Lazy Evaluation**: Only calculate influence when regions need it

**Scalability**:
- System designed for <1000 active heroes
- If hero count grows, can implement hero LOD (but unlikely needed)
- Influence calculations are O(heroes × regions), but heroes are rare

### Hero Events

**Hero-Specific Events**:
- Hero promotion celebrations
- Hero accomplishments (broadcast to influenced regions)
- Hero conflicts (hero vs hero, hero vs major threat)
- Hero lineage events (children of heroes)
- Hero death and succession

**Integration with Event System**:
- Heroes can trigger regional and global events
- Hero actions have higher event priority
- Hero-related events propagate to influenced regions
```
region_skill_distribution[region_id][skill_id] = statistical_model
// No individual tracking, only population-level statistics
```

### Skill Progression: Probabilistic System

**Core Design**: Skills progress purely by chance - no experience tracking
- Only current skill level is stored (0-15)
- Progression happens when random chance succeeds
- More calculations per update, but minimal memory overhead

**Progression Probability Calculation**:

Each update cycle, for each skill, calculate probability of leveling up:

```
base_probability = skill_progression_curve(current_level)
modified_probability = base_probability * trait_modifier * regional_modifier * event_modifier * age_modifier * activity_modifier

// Roll random number [0.0, 1.0)
if (random() < modified_probability && current_level < 15):
    current_level += 1
```

**Progression Curve** (All configurable in `skills.progression`):
- Level 0→1: `skills.progression.base_probability_level_0` (default: 0.1 per day for active skills)
- Level 5→6: `skills.progression.base_probability_level_5` (default: 0.01 per day)
- Level 8→9: `skills.progression.base_probability_level_8` (default: 0.001 per day)
- Level 9→10: `skills.progression.base_probability_level_9` (default: 0.0001 per day) - **Mortal cap**
- Level 10→11: `skills.progression.base_probability_level_10` (default: 0.00001 per day) - **Divine level** (if enabled)
- Level 14→15: `skills.progression.base_probability_level_14` (default: 0.000001 per day) - **Divine level** (if enabled)
- Curve shape controlled by `skills.progression.progression_curve_exponent`

**Mortal Cap Enforcement**:
- If `skills.divine_levels_enabled` = false (default):
  - Skills cannot progress beyond `skills.mortal_max_level` (default: 9)
  - Level 9 is the absolute maximum for any mortal being
  - Divine levels (10-15) are completely inaccessible
- If `skills.divine_levels_enabled` = true:
  - Mortals can theoretically reach divine levels (extremely rare)
  - Requires exponential progression rates
  - May represent special events, divine intervention, or exceptional circumstances

**Modifiers** (all multiply base probability):

1. **Race Modifiers**: Species-specific traits
   - Base progression: `races.races[race_id].skill_progression_multiplier` (e.g., elves: 0.6x, orcs: 1.2x)
   - Skill affinity: `races.races[race_id].skill_affinities[skill_id]` (e.g., elves +1.5x nature magic)
   - Skill penalty: `races.races[race_id].skill_penalties[skill_id]` (e.g., orcs 0.7x magic)
   - **Applied first** before other modifiers

2. **Trait Modifiers**: Genetic/cultural aptitude
   - Natural talent: 1.5x to 2.0x
   - Affinity for skill type: 1.2x
   - Regional specialty: 1.3x

3. **Regional Modifiers**: Environmental factors
   - Skill availability/resources: 0.5x to 2.0x
   - Cultural emphasis: 1.1x to 1.5x
   - Magic/mana density (for magical skills): 0.8x to 2.0x

4. **Event Modifiers**: Temporary effects
   - Training events: 3.0x to 10.0x
   - Disaster/conflict: -0.5x (skills regress) or +2.0x (combat skills)
   - Discovery/inspiration: 5.0x for specific skills

5. **Age Modifiers**: Life stage effects (race-adjusted)
   - Childhood (0-12): +1.5x learning rate, -0.5x physical skills
   - Adolescence (13-18): +1.2x all skills
   - Prime (19-40): Baseline (1.0x)
   - Middle age (41-60): +0.8x mental, -0.7x physical
   - Elder (61+): -0.5x physical, +0.9x knowledge
   - **Note**: Age ranges may be adjusted based on race lifespan

6. **Activity Modifiers**: Usage-based (no tracking, just current activity)
   - Currently using skill: 10.0x to 50.0x
   - Related activity: 2.0x to 5.0x
   - Inactive: 0.1x to 0.5x (slow decay/chance of leveling)

**Final Calculation**:
```
base_prob = progression_curve(current_level)
race_base = base_prob * races[race_id].skill_progression_multiplier
race_affinity = race_base * (races[race_id].skill_affinities[skill_id] or 1.0)
race_penalty = race_affinity * (races[race_id].skill_penalties[skill_id] or 1.0)
final_prob = race_penalty * trait_mod * regional_mod * event_mod * age_mod * activity_mod

// Check mortal cap
if (current_level >= skills.mortal_max_level && !skills.divine_levels_enabled):
    final_prob = 0.0  // Cannot progress beyond mortal max
```

**Performance Considerations - Computational Trade-offs**:

**Memory vs CPU Trade-off**:
- **Memory Saved**: 300 bytes per entity (75% reduction vs u16)
- **CPU Cost**: Additional calculations per update (probabilistic progression)
- **Net Benefit**: Memory is often the bottleneck at 1M scale, CPU calculations are fast and parallelizable

**Full Simulation**: Calculate for all skills of all entities in focus regions
- **Raw Calculations**: 200 skills × N entities = 200N probability calculations per update
- **Example**: 10,000 entities = 2,000,000 calculations per update
- **Optimization Strategies**:
  1. **Early Exit**: Skip calculation if base probability below threshold (e.g., < 0.0001)
     - Most high-level skills won't progress each frame
     - Can reduce calculations by 80-90%
  2. **Batch Random Number Generation**: Generate random numbers in bulk (faster than individual calls)
  3. **SIMD Parallelization**: Process multiple entities simultaneously
     - Calculate probabilities for 4-8 entities at once
  4. **Cache-Friendly Processing**: Process entities in chunks
     - Keep skill arrays for chunk in L1/L2 cache
  5. **Conditional Updates**: Only calculate for active/used skills
     - Track "recently used" flags (simple bitfield)
     - Only process skills that have activity modifiers
  6. **Staggered Updates**: Update different skill categories on different frames
     - Combat skills frame N, crafting skills frame N+1, etc.
  7. **Lookup Tables**: Pre-compute common probability values
     - Base probabilities by level stored in LUT
     - Fast multiplication-based modifiers

**Half Simulation**: Reduced calculation load
- **Strategy 1**: Calculate for representative sample (10-20% of population)
- **Strategy 2**: Simplified probability formulas (fewer modifiers)
- **Strategy 3**: Update fewer skills per entity (e.g., 50 most relevant)
- **Strategy 4**: Reduce update frequency (every 2-4 ticks)

**Formula Simulation**: Minimal calculations
- **Approach**: Aggregate probability models
- **Formula**: Expected skill distribution changes over time based on regional modifiers
- **Calculation**: O(1) per skill per region, not per entity
- **Example**: `region_skill_avg[skill_id] += regional_growth_rate[skill_id] * delta_time`

**Estimated Performance Impact**:
- With optimizations: ~10-50 microseconds per 1000 entities (full sim)
- At 60 FPS: Can handle 20,000-100,000 entities in full simulation (depending on optimizations)
- Memory savings allow more entities in focus, justifying computational cost

**Skill Decay** (Optional):

Higher level skills might have small chance of decay if unused:
```
if (skill_inactive && random() < decay_probability(current_level)):
    current_level = max(0, current_level - 1)
```

This ensures population skill distributions remain dynamic and realistic.

---

## Event System

### Event Types

1. **Global Events**: Affect entire world
   - Natural disasters
   - Wars
   - Major discoveries
   - Economic shifts

2. **Regional Events**: Affect single region
   - Local disasters
   - Cultural shifts
   - Resource discoveries
   - Political changes

3. **Individual Events**: Affect specific entities
   - Personal achievements
   - Relationships
   - Life events
   - Skill breakthroughs

### Event Propagation

**Priority Queue System**:
- Events scheduled with timestamps
- Processed in order
- Can trigger cascading events

**Impact Calculation**:
- Full sim: Direct entity updates
- Half sim: Statistical adjustments
- Formula sim: Probability-based outcomes

### Event Storage

**Active Events**: In-memory priority queue
**Event History**: Compressed log (for replay/debugging)
**Event Templates**: Predefined event types with parameters

---

## Technical Specifications

### Performance Targets

| Metric | Target (Configurable) | Default Value |
|--------|----------------------|---------------|
| Frame Time | `performance.target_frame_time_ms` | <16.67ms (60 FPS) |
| Memory per Entity | `memory.target_memory_per_entity_bytes` | <200 bytes |
| Skill Storage per Entity | `skills.skill_count` × 0.5 bytes | 100 bytes (200×4-bit) |
| LOD Transition Time | Configurable | <100ms (acceptable: <500ms) |
| Save/Load Time | Configurable | <5s for 1M entities (acceptable: <10s) |
| Skill Progression Calculations | Optimized batches | Per-entity loops acceptable |
| Thread Count | `performance.thread_count` (0 = auto) | Auto-detect CPU cores |

### Technology Recommendations

**Language**: C++ or Rust (performance-critical)
- C++: Mature, lots of libraries, manual memory management
- Rust: Safety guarantees, good performance, modern tooling

**Alternative**: C# with Unity DOTS (Data-Oriented Technology Stack)
- Good for rapid prototyping
- Built-in ECS support
- Cross-platform

**Data Storage**:
- In-memory: Custom SoA structures
- Persistence: Binary format with compression
- Optional: Database for analytics/debugging

### Memory Budget

**Per-Entity Breakdown** (at default settings: 1M entities, 200 skills):
- Skills: `skills.skill_count` × 0.5 bytes = **N/2 bytes** (default: 100 bytes)
- ID: 8 bytes (u64)
- Region ID: 1 byte (u8, max `world.region_count` ≤ 255)
- Age: 2 bytes (u16, max `simulation.entity.max_age`)
- Traits: ~4-8 bytes (bitfield, includes `is_hero` flag)
- Position (if needed): 8 bytes (2× f32)
- Renown: 2 bytes (u16, sparse - only for heroes, ~`heroes.hero_percentage` of entities)
- Other core data: ~20-50 bytes
- **Subtotal**: ~140-180 bytes per entity (regular, at default settings)
- **Hero Overhead**: +2 bytes renown (but only `heroes.max_heroes` heroes)
- **Total**: ~140-180 MB for 1M entities (just entity data, at default settings)
- **Target**: `memory.target_memory_per_entity_bytes` (default: 200 bytes)

**Additional Systems** (at default settings):
- Regional data: `world.region_count` regions × ~1KB = ~N KB (default: 100 KB)
- Hero system: ~50-100 bytes × `heroes.max_heroes` = ~N KB (default: 50-100 KB)
- Event system: ~10-50 MB (active events + history, max: `events.max_active_events`)
- Spatial indexing: ~50-100 MB
- System overhead: ~100-200 MB
- **Subtotal**: ~200-350 MB (at default settings)
- **Total Target**: `memory.max_memory_mb` (default: 4096 MB)

**Total Target**: **~400-600 MB** for core simulation data (significantly under 1GB target)

**Optimization Techniques**:
- 4-bit skill storage (200 skills in 100 bytes vs 400 bytes with u16)
- Compress inactive data
- Use smaller data types where possible (u8 vs u32, u4 for skills)
- Bitfields for flags and traits
- Sparse arrays for optional data
- Packed data structures (skills, traits)

---

## Conclusion

This design document provides a roadmap for building a massive-scale simulation game. The key to success is:

1. **Start small, scale gradually**: Build MVP first, expand systematically
2. **Performance from the start**: Use efficient data structures from day one
3. **LOD is essential**: Different simulation levels are not optional
4. **Profile constantly**: Know where time is spent
5. **Iterate on architecture**: Be willing to refactor as scale increases

The ultimate goal of 1 million entities is ambitious but achievable with careful architecture, efficient data structures, and intelligent LOD systems.

---

**Document Version**: 1.4  
**Last Updated**: Added Race System and updated Skill System (mortal max level 9, divine levels 10-15)  
**Status**: Design Phase

