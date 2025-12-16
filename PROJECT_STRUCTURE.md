# Project Structure

This document outlines the complete class structure, methods, and directory layout for the Fantasy Sim C++ project.

## Directory Structure

```
fantasy_sim/
├── CMakeLists.txt          # CMake build configuration
├── vcpkg.json              # vcpkg dependency manifest
├── README.md               # Project readme
├── DESIGN_DOCUMENT.md      # Game design document
├── PROJECT_STRUCTURE.md    # This file
│
├── include/                # Header files
│   ├── Core/               # Core types and configuration
│   │   ├── Types.h         # Type aliases and enums
│   │   └── Config.h        # Configuration system
│   │
│   ├── Platform/           # Platform abstraction layer
│   │   ├── IVideo.h        # Video/rendering interface
│   │   ├── IInput.h        # Input interface
│   │   ├── SDLVideo.h      # SDL video implementation
│   │   ├── SDLInput.h      # SDL input implementation
│   │   └── PlatformFactory.h # Factory for creating implementations
│   │
│   ├── ECS/                # Entity Component System
│   │   ├── Entity.h        # Entity manager
│   │   ├── Component.h     # Component manager (SoA)
│   │   └── System.h        # System manager and coordinator
│   │
│   ├── Components/         # ECS Components
│   │   ├── Transform.h     # Position component
│   │   ├── Inhabitant.h    # Core entity data
│   │   ├── Skills.h        # Skills component (4-bit packed)
│   │   ├── Hero.h          # Hero component
│   │   └── Renown.h        # Renown component
│   │
│   ├── Systems/            # ECS Systems
│   │   ├── AgingSystem.h           # Aging and death
│   │   ├── SkillProgressionSystem.h # Skill leveling
│   │   ├── BirthDeathSystem.h       # Birth/death events
│   │   ├── MigrationSystem.h        # Region migration
│   │   └── HeroSystem.h             # Hero updates
│   │
│   ├── Simulation/         # Simulation management
│   │   ├── SimulationManager.h  # Main simulation orchestrator
│   │   ├── LODSystem.h          # Level of Detail system
│   │   └── Region.h             # Region class
│   │
│   ├── Race/               # Race system
│   │   └── RaceManager.h  # Race definitions and lookups
│   │
│   ├── Skills/             # Skill system
│   │   └── SkillSystem.h  # Skill progression logic
│   │
│   ├── Heroes/             # Hero system
│   │   └── HeroSystem.h   # Hero management and renown
│   │
│   ├── Events/             # Event system
│   │   └── EventSystem.h  # Event scheduling and processing
│   │
│   ├── Utils/              # Utility classes
│   │   ├── Random.h        # Random number generation
│   │   ├── MemoryPool.h    # Memory pool allocator
│   │   └── Profiler.h      # Performance profiler
│   │
│   ├── Data/               # Data structures
│   │   └── EntityStorage.h # SoA entity storage
│   │
│   └── Game/               # Main game class
│       └── Game.h          # Game loop and initialization
│
├── src/                    # Source files
│   └── main.cpp           # Entry point
│
└── config/                 # Configuration files
    └── default.json        # Default configuration
```

## Core Classes and Methods

### Core System

#### `Config::Configuration`
**Location**: `include/Core/Config.h`

**Methods**:
- `static Configuration& GetInstance()` - Singleton access
- `bool LoadFromFile(const std::string& filepath)` - Load config from JSON file
- `bool LoadFromJSON(const std::string& jsonString)` - Load config from JSON string
- `bool SaveToFile(const std::string& filepath) const` - Save config to file
- `std::string SaveToJSON() const` - Serialize to JSON
- `bool Validate() const` - Validate configuration values
- `bool Reload()` - Hot reload configuration

**Configuration Sections**:
- `WorldConfig world` - World settings
- `PerformanceConfig performance` - Performance settings
- `SimulationConfig simulation` - Simulation parameters
- `SkillsConfig skills` - Skill system config
- `HeroesConfig heroes` - Hero system config
- `RacesConfig races` - Race definitions
- `RegionsConfig regions` - Region settings
- `EventsConfig events` - Event system config
- `MemoryConfig memory` - Memory management
- `RenderingConfig rendering` - Rendering settings

### ECS System

#### `ECS::EntityManager`
**Location**: `include/ECS/Entity.h`

**Methods**:
- `Entity CreateEntity()` - Create new entity
- `void DestroyEntity(Entity entity)` - Destroy entity
- `bool IsValid(Entity entity) const` - Check validity
- `u32 GetEntityCount() const` - Get total count
- `void Reset()` - Clear all entities

#### `ECS::ComponentManager`
**Location**: `include/ECS/Component.h`

**Methods**:
- `template<typename T> void RegisterComponent()` - Register component type
- `template<typename T> void AddComponent(EntityID, const T&)` - Add component
- `template<typename T> void RemoveComponent(EntityID)` - Remove component
- `template<typename T> T* GetComponent(EntityID)` - Get component
- `template<typename T> bool HasComponent(EntityID) const` - Check component
- `void OnEntityDestroyed(EntityID)` - Cleanup on destroy

#### `ECS::SystemManager`
**Location**: `include/ECS/System.h`

**Methods**:
- `template<typename T> std::shared_ptr<T> RegisterSystem()` - Register system
- `template<typename T> void SetSignature(Signature)` - Set required components
- `void Update(f32 delta_time)` - Update all systems
- `void OnEntityDestroyed(EntityID)` - Entity cleanup
- `void OnEntitySignatureChanged(EntityID, Signature)` - Signature change

#### `ECS::Coordinator`
**Location**: `include/ECS/System.h`

**Methods**:
- `static Coordinator& GetInstance()` - Singleton access
- `EntityID CreateEntity()` - Create entity
- `void DestroyEntity(EntityID)` - Destroy entity
- `template<typename T> void RegisterComponent()` - Register component
- `template<typename T> void AddComponent(EntityID, const T&)` - Add component
- `template<typename T> void RemoveComponent(EntityID)` - Remove component
- `template<typename T> T* GetComponent(EntityID)` - Get component
- `template<typename T> bool HasComponent(EntityID)` - Check component
- `template<typename T> std::shared_ptr<T> RegisterSystem()` - Register system
- `template<typename T> void SetSystemSignature(Signature)` - Set signature
- `void Update(f32 delta_time)` - Update all systems

### Components

#### `Components::Inhabitant`
**Location**: `include/Components/Inhabitant.h`

**Data Members**:
- `EntityID id` - Entity identifier
- `RegionID region_id` - Current region
- `RaceID race_id` - Race identifier
- `u16 age` - Age in years
- `u8 traits` - Packed traits bitfield

**Methods**:
- `bool IsHero() const` - Check if hero
- `void SetHero(bool)` - Set hero flag

#### `Components::Skills`
**Location**: `include/Components/Skills.h`

**Methods**:
- `u8 GetSkill(SkillID) const` - Get skill level (0-15)
- `void SetSkill(SkillID, u8)` - Set skill level
- `bool IncrementSkill(SkillID, u8 max_level)` - Increment skill
- `bool DecrementSkill(SkillID, u8 min_level)` - Decrement skill
- `u16 GetSkillCount() const` - Get total skills
- `const u8* GetData() const` - Get raw packed data
- `size_t GetDataSize() const` - Get data size in bytes
- `void Reset()` - Reset all skills to 0
- `u32 GetTotalSkillPoints() const` - Sum of all skills
- `u8 GetHighestSkillLevel() const` - Highest skill level
- `u16 GetSkillsAtOrAboveLevel(u8) const` - Count skills at level

**Storage**: 4-bit packed (2 skills per byte)

#### `Components::Hero`
**Location**: `include/Components/Hero.h`

**Data Members**:
- `EntityID entity_id` - Entity reference
- `u16 renown` - Renown value
- `u8 influence_radius` - Influence radius in regions
- `u8 hero_type` - Hero type (Combat/Crafting/Knowledge/Social/Mixed)
- `std::vector<RegionID> influenced_regions` - Affected regions
- `std::vector<f32> influence_strengths` - Influence per region
- `Tick last_accomplishment_tick` - Last achievement time
- `Tick promotion_tick` - When became hero

**Methods**:
- `bool IsLocalHero() const` - Check tier
- `bool IsRegionalHero() const`
- `bool IsNationalHero() const`
- `bool IsLegendaryHero() const`
- `const char* GetTierName() const` - Get tier name

### Systems

#### `Systems::AgingSystem`
**Location**: `include/Systems/AgingSystem.h`

**Methods**:
- `void Update(f32 delta_time)` - Update aging
- `bool ShouldAge(EntityID, f32) const` - Check if should age
- `void AgeEntity(EntityID, f32)` - Age entity
- `bool ShouldDie(EntityID) const` - Check death condition
- `void KillEntity(EntityID)` - Kill entity

#### `Systems::SkillProgressionSystem`
**Location**: `include/Systems/SkillProgressionSystem.h`

**Methods**:
- `void Update(f32 delta_time)` - Update skill progression
- `void UpdateEntitySkills(EntityID, f32)` - Update single entity
- `void BatchUpdateSkills(const std::vector<EntityID>&, f32)` - Batch update

#### `Systems::BirthDeathSystem`
**Location**: `include/Systems/BirthDeathSystem.h`

**Methods**:
- `void Update(f32 delta_time)` - Process births/deaths
- `EntityID CreateNewEntity(RegionID, RaceID, EntityID parent1, EntityID parent2)` - Create entity
- `void ProcessBirths(RegionID, f32)` - Process region births
- `void ProcessDeaths(RegionID, f32)` - Process region deaths

#### `Systems::MigrationSystem`
**Location**: `include/Systems/MigrationSystem.h`

**Methods**:
- `void Update(f32 delta_time)` - Process migrations
- `bool ShouldMigrate(EntityID) const` - Check migration
- `bool MigrateEntity(EntityID, RegionID)` - Migrate entity
- `RegionID FindMigrationTarget(EntityID) const` - Find best target

### Simulation System

#### `Simulation::SimulationManager`
**Location**: `include/Simulation/SimulationManager.h`

**Methods**:
- `bool Initialize()` - Initialize simulation
- `void Update(f32 delta_time)` - Update simulation
- `void SetFocusRegions(const std::vector<RegionID>&)` - Set focus
- `std::vector<RegionID> GetFocusRegions() const` - Get focus
- `Region* GetRegion(RegionID)` - Get region
- `const std::vector<std::unique_ptr<Region>>& GetRegions() const` - Get all
- `Tick GetCurrentTick() const` - Get current tick
- `void Pause()` - Pause simulation
- `void Resume()` - Resume simulation
- `bool IsPaused() const` - Check paused state
- `void SetTimeScale(f32)` - Set time scale
- `f32 GetTimeScale() const` - Get time scale

#### `Simulation::LODSystem`
**Location**: `include/Simulation/LODSystem.h`

**Methods**:
- `void Initialize()` - Initialize LOD
- `void UpdateLOD(const std::vector<RegionID>&, u8)` - Update LOD assignments
- `SimulationLOD GetRegionLOD(RegionID) const` - Get region LOD
- `bool ShouldUpdateRegion(RegionID, Tick) const` - Check update
- `std::vector<RegionID> GetRegionsAtLOD(SimulationLOD) const` - Get regions
- `void TransitionRegion(RegionID, SimulationLOD)` - Transition LOD

#### `Simulation::Region`
**Location**: `include/Simulation/Region.h`

**Methods**:
- `void Initialize()` - Initialize region
- `void Update(f32, SimulationLOD, Tick)` - Update region
- `RegionID GetID() const` - Get ID
- `const std::string& GetType() const` - Get type
- `u32 GetPopulation() const` - Get population
- `u32 GetCapacity() const` - Get capacity
- `void AddEntity(EntityID)` - Add entity
- `void RemoveEntity(EntityID)` - Remove entity
- `bool IsAtCapacity() const` - Check capacity
- `f32 GetResource(const std::string&) const` - Get resource
- `void SetResource(const std::string&, f32)` - Set resource
- `void ModifyResource(const std::string&, f32)` - Modify resource
- `void SetTrait(u8, u8, bool)` - Set trait
- `bool GetTrait(u8, u8) const` - Get trait
- `void AddNeighbor(RegionID)` - Add neighbor
- `const std::vector<RegionID>& GetNeighbors() const` - Get neighbors
- `void SetPosition(f32, f32)` - Set position
- `f32 GetX() const` - Get X
- `f32 GetY() const` - Get Y
- `void AddHeroInfluence(EntityID, f32)` - Add hero influence
- `void RemoveHeroInfluence(EntityID)` - Remove influence
- `f32 GetHeroInfluence(EntityID) const` - Get influence
- `void UpdateSkillDistribution(SkillID, f32, f32)` - Update stats
- `f32 GetSkillMean(SkillID) const` - Get mean
- `f32 GetSkillStdDev(SkillID) const` - Get std dev

### Race System

#### `Race::RaceManager`
**Location**: `include/Race/RaceManager.h`

**Methods**:
- `static RaceManager& GetInstance()` - Singleton access
- `bool Initialize(const Config::RacesConfig&)` - Initialize
- `const Config::RaceDefinition* GetRace(RaceID) const` - Get race
- `const Config::RaceDefinition* GetRaceByName(const std::string&) const` - Get by name
- `const std::vector<Config::RaceDefinition>& GetAllRaces() const` - Get all
- `f32 GetAgingRate(RaceID) const` - Get aging rate
- `u16 GetMaxAge(RaceID) const` - Get max age
- `f32 GetSkillProgressionMultiplier(RaceID) const` - Get multiplier
- `f32 GetSkillAffinity(RaceID, SkillID) const` - Get affinity
- `f32 GetSkillPenalty(RaceID, SkillID) const` - Get penalty
- `f32 GetRegionAttraction(RaceID, const std::string&) const` - Get attraction
- `RaceID DetermineOffspringRace(RaceID, RaceID) const` - Determine offspring
- `RaceID GetRandomRace() const` - Get random race

### Skill System

#### `Skills::SkillSystem`
**Location**: `include/Skills/SkillSystem.h`

**Methods**:
- `void Initialize()` - Initialize system
- `void UpdateSkillProgression(Components::Skills&, const Components::Inhabitant&, f32, const std::vector<bool>&)` - Update progression
- `f32 CalculateProgressionProbability(u8, RaceID, SkillID, u16, bool, bool, const std::vector<f32>&) const` - Calculate probability
- `f32 GetBaseProbability(u8) const` - Get base probability
- `f32 GetAgeModifier(u16, RaceID) const` - Get age modifier
- `bool CanProgress(u8, bool, u8) const` - Check if can progress

### Hero System

#### `Heroes::HeroSystem`
**Location**: `include/Heroes/HeroSystem.h`

**Methods**:
- `void Initialize()` - Initialize system
- `void Update(f32, Tick)` - Update heroes
- `bool CheckAndPromote(EntityID, u16)` - Check promotion
- `void AwardRenown(EntityID, u16, const std::string&)` - Award renown
- `Components::Hero* GetHero(EntityID)` - Get hero data
- `bool IsHero(EntityID) const` - Check if hero
- `std::vector<EntityID> GetAllHeroes() const` - Get all heroes
- `u16 CalculateRenownFromSkills(const Components::Skills&) const` - Calculate renown
- `void AwardCombatRenown(EntityID, const std::string&, u16)` - Combat renown
- `void AwardSkillRenown(EntityID, SkillID, u8)` - Skill renown
- `void AwardLineageRenown(EntityID, EntityID, EntityID)` - Lineage renown
- `void AwardAccomplishmentRenown(EntityID, const std::string&)` - Accomplishment
- `void UpdateHeroInfluences()` - Update influences
- `const char* GetHeroTier(u16) const` - Get tier
- `u8 GetInfluenceRadius(u16) const` - Get radius
- `f32 CalculateInfluenceStrength(u16, u8) const` - Calculate strength

### Event System

#### `Events::EventSystem`
**Location**: `include/Events/EventSystem.h`

**Methods**:
- `void Initialize()` - Initialize system
- `void Update(f32, Tick)` - Update events
- `EventID ScheduleEvent(std::unique_ptr<Event>, Tick)` - Schedule event
- `EventID ScheduleImmediateEvent(std::unique_ptr<Event>)` - Immediate event
- `bool CancelEvent(EventID)` - Cancel event
- `Event* GetEvent(EventID)` - Get event
- `void RegisterHandler(const std::string&, std::function<void(Event&)>)` - Register handler
- `std::unique_ptr<Event> CreateGlobalEvent(const std::string&)` - Create global
- `std::unique_ptr<Event> CreateRegionalEvent(const std::string&, RegionID)` - Create regional
- `std::unique_ptr<Event> CreateIndividualEvent(const std::string&, EntityID)` - Create individual
- `const std::vector<std::unique_ptr<Event>>& GetEventHistory() const` - Get history
- `u32 GetActiveEventCount() const` - Get active count

### Utilities

#### `Utils::Random`
**Location**: `include/Utils/Random.h`

**Methods**:
- `static Random& GetInstance()` - Singleton access
- `void Seed(u64)` - Seed RNG
- `void Seed()` - Random seed
- `f32 RandomFloat()` - Random float [0,1)
- `f32 RandomFloat(f32, f32)` - Random float [min,max)
- `u32 RandomU32()` - Random u32
- `u32 RandomU32(u32, u32)` - Random u32 [min,max]
- `u64 RandomU64()` - Random u64
- `i32 RandomI32(i32, i32)` - Random i32 [min,max]
- `bool RandomBool(f32)` - Random bool with probability
- `template<typename Container> auto RandomChoice(const Container&)` - Random choice

#### `Utils::Profiler`
**Location**: `include/Utils/Profiler.h`

**Methods**:
- `static Profiler& GetInstance()` - Singleton access
- `void StartSection(const std::string&)` - Start profiling
- `void EndSection(const std::string&)` - End profiling
- `f32 GetSectionTime(const std::string&) const` - Get time
- `std::unordered_map<std::string, f32> GetAllSectionTimes() const` - Get all
- `void Reset()` - Reset timings
- `void PrintReport() const` - Print report

**Macros**:
- `PROFILE_SCOPE(name)` - RAII profiler scope
- `PROFILE_START(name)` - Start section
- `PROFILE_END(name)` - End section

### Platform Abstraction

#### `Platform::IVideo`
**Location**: `include/Platform/IVideo.h`

**Abstract Interface** for video/rendering backends. Allows swapping SDL, OpenGL, DirectX, etc.

**Methods**:
- `bool Initialize()` - Initialize video system
- `void Shutdown()` - Shutdown video system
- `bool CreateWindow(const std::string&, i32, i32, bool)` - Create window
- `void DestroyWindow()` - Destroy window
- `void BeginFrame()` - Begin rendering frame
- `void EndFrame()` - End frame (present)
- `void Clear(u8, u8, u8, u8)` - Clear screen
- `void SetDrawColor(u8, u8, u8, u8)` - Set draw color
- `void DrawRect(i32, i32, i32, i32)` - Draw filled rectangle
- `void DrawRectOutline(i32, i32, i32, i32)` - Draw rectangle outline
- `void DrawLine(i32, i32, i32, i32)` - Draw line
- `void DrawPoint(i32, i32)` - Draw point
- `i32 GetWindowWidth() const` - Get window width
- `i32 GetWindowHeight() const` - Get window height
- `bool ShouldClose() const` - Check if window should close
- `void SetWindowTitle(const std::string&)` - Set window title
- `void SetFullscreen(bool)` - Toggle fullscreen
- `void GetWindowSize(i32&, i32&) const` - Get window size
- `void SetViewport(i32, i32, i32, i32)` - Set viewport
- `void ResetViewport()` - Reset viewport

#### `Platform::IInput`
**Location**: `include/Platform/IInput.h`

**Abstract Interface** for input backends. Allows swapping SDL, GLFW, Win32, etc.

**Methods**:
- `bool Initialize()` - Initialize input system
- `void Shutdown()` - Shutdown input system
- `void Update()` - Update input state (call per frame)
- `const InputState& GetState() const` - Get current input state
- `bool IsKeyDown(KeyCode) const` - Check if key is held
- `bool IsKeyPressed(KeyCode) const` - Check if key was pressed this frame
- `bool IsKeyReleased(KeyCode) const` - Check if key was released this frame
- `bool IsMouseButtonDown(MouseButton) const` - Check if mouse button is held
- `bool IsMouseButtonPressed(MouseButton) const` - Check if mouse button pressed
- `bool IsMouseButtonReleased(MouseButton) const` - Check if mouse button released
- `void GetMousePosition(i32&, i32&) const` - Get mouse position
- `void GetMouseDelta(i32&, i32&) const` - Get mouse delta
- `void GetMouseWheel(i32&, i32&) const` - Get mouse wheel
- `bool IsWindowFocused() const` - Check window focus
- `bool IsWindowMinimized() const` - Check if minimized
- `void StartTextInput()` - Start text input
- `void StopTextInput()` - Stop text input
- `bool IsTextInputActive() const` - Check text input active
- `std::string GetTextInput() const` - Get text input
- `void ClearTextInput()` - Clear text input
- `std::string GetKeyName(KeyCode) const` - Get key name

#### `Platform::SDLVideo`
**Location**: `include/Platform/SDLVideo.h`

**SDL2 implementation** of `IVideo` interface.

**Additional Methods**:
- `SDL_Window* GetSDLWindow() const` - Get SDL window (for SDL-specific code)
- `SDL_Renderer* GetSDLRenderer() const` - Get SDL renderer

#### `Platform::SDLInput`
**Location**: `include/Platform/SDLInput.h`

**SDL2 implementation** of `IInput` interface.

#### `Platform::PlatformFactory`
**Location**: `include/Platform/PlatformFactory.h`

**Factory** for creating platform implementations.

**Methods**:
- `static std::unique_ptr<IVideo> CreateVideo(VideoBackend)` - Create video backend
- `static std::unique_ptr<IInput> CreateInput(InputBackend)` - Create input backend
- `static std::unique_ptr<IVideo> CreateVideoFromString(const std::string&)` - Create from string
- `static std::unique_ptr<IInput> CreateInputFromString(const std::string&)` - Create from string
- `static std::string GetDefaultVideoBackendName()` - Get default video backend name
- `static std::string GetDefaultInputBackendName()` - Get default input backend name

### Game

#### `Game::Game`
**Location**: `include/Game/Game.h`

**Methods**:
- `bool Initialize()` - Initialize game
- `void Run()` - Run game loop
- `void Shutdown()` - Shutdown game
- `bool LoadConfig(const std::string&)` - Load config
- `bool SaveGame(const std::string&)` - Save game
- `bool LoadGame(const std::string&)` - Load game
- `void SetVideo(std::unique_ptr<Platform::IVideo>)` - Set video implementation
- `void SetInput(std::unique_ptr<Platform::IInput>)` - Set input implementation

**Private Methods**:
- `void Update(f32)` - Update game state
- `void Render()` - Render game
- `void ProcessInput()` - Handle input
- `bool InitializePlatform()` - Initialize platform systems
- `void InitializeECS()` - Initialize ECS
- `void InitializeSystems()` - Initialize systems

## Build System

### CMake Configuration
- **CMakeLists.txt**: Main build configuration
- **vcpkg.json**: Dependency manifest (SDL2, nlohmann-json)
- **C++ Standard**: C++20
- **Platform Support**: Windows, Linux, macOS

### Dependencies
- **SDL2**: Window management and rendering
- **SDL2_image**: Image loading
- **SDL2_ttf**: Font rendering
- **nlohmann-json**: JSON configuration parsing

## Configuration Files

### default.json
Contains all default configuration values as specified in the design document:
- World settings
- Performance settings
- Simulation parameters
- Skill system config
- Hero system config
- Race definitions
- Region settings
- Event system config
- Memory settings
- Rendering settings

## Design Principles

1. **ECS Architecture**: Entity Component System for flexibility and performance
2. **SoA Storage**: Structure of Arrays for cache-friendly batch processing
3. **Configuration-Driven**: No hard-coded values, all configurable
4. **LOD System**: Full/Half/Formula simulation levels
5. **Memory Efficiency**: 4-bit skill storage, sparse hero data
6. **Cross-Platform**: CMake + vcpkg for OS-agnostic builds
7. **Performance**: SIMD support, batch processing, profiling



