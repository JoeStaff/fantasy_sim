# Quick Start Guide

## Project Overview

This is a C++ implementation of the Fantasy Sim game, designed to simulate 1 million entities across 100 regions with 200 skills each.

## Key Features

- **ECS Architecture**: Entity Component System for flexible, performant design
- **SoA Storage**: Structure of Arrays for cache-friendly batch processing
- **LOD System**: Full/Half/Formula simulation levels for performance
- **Configuration-Driven**: All parameters configurable via JSON
- **Cross-Platform**: Windows, Linux, macOS support via CMake
- **Platform Abstraction**: Swappable video and input backends (SDL, future: OpenGL, DirectX, etc.)

## Building the Project

### Prerequisites
1. CMake 3.20+
2. C++20 compiler (MSVC 2019+, GCC 10+, Clang 12+)
3. vcpkg for dependency management

### Setup Steps

1. **Install vcpkg** (if not installed):
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # Linux/Mac
.\bootstrap-vcpkg.bat  # Windows
```

2. **Install dependencies**:
```bash
vcpkg install sdl2 sdl2-image sdl2-ttf nlohmann-json
```

3. **Configure and build**:
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Project Structure Summary

### Core Systems
- **Config**: JSON-based configuration system
- **Platform**: Video and input abstraction layer (swappable backends)
- **ECS**: Entity Component System framework
- **Simulation**: LOD-based simulation manager
- **Race**: Race definitions and modifiers
- **Skills**: Skill progression system
- **Heroes**: Hero and renown system
- **Events**: Event scheduling and processing

### Components
- `Inhabitant`: Core entity data (region, race, age, traits)
- `Skills`: 200 skills stored as 4-bit packed values
- `Hero`: Hero-specific data (renown, influence)
- `Transform`: Position in world
- `Renown`: Sparse renown storage

### Systems
- `AgingSystem`: Handles aging and death
- `SkillProgressionSystem`: Probabilistic skill leveling
- `BirthDeathSystem`: Population management
- `MigrationSystem`: Region migration
- `HeroSystem`: Hero updates and influences

## Key Classes Quick Reference

### Configuration
```cpp
auto& config = Config::Configuration::GetInstance();
u32 max_pop = config.world.max_population;
f32 fps = config.performance.target_fps;
```

### ECS
```cpp
auto& coordinator = ECS::Coordinator::GetInstance();
EntityID entity = coordinator.CreateEntity();
coordinator.AddComponent<Components::Inhabitant>(entity, inhabitant);
auto* skills = coordinator.GetComponent<Components::Skills>(entity);
```

### Simulation
```cpp
Simulation::SimulationManager sim;
sim.Initialize();
sim.Update(delta_time);
Region* region = sim.GetRegion(region_id);
```

### Skills
```cpp
Components::Skills skills(200);  // 200 skills
skills.SetSkill(0, 5);  // Set skill 0 to level 5
u8 level = skills.GetSkill(0);  // Get skill 0 level
```

### Heroes
```cpp
Heroes::HeroSystem hero_system;
hero_system.AwardRenown(entity_id, 100, "combat");
bool is_hero = hero_system.IsHero(entity_id);
```

### Random
```cpp
auto& rng = Utils::Random::GetInstance();
f32 prob = rng.RandomFloat();  // [0, 1)
bool result = rng.RandomBool(0.5f);  // 50% chance
```

## Configuration

Edit `config/default.json` to customize:
- Population size
- Region count
- Skill count
- Performance targets
- Race definitions
- Hero thresholds
- And more...

See `DESIGN_DOCUMENT.md` for complete configuration reference.

## Performance Considerations

- **Memory**: ~200 bytes per entity (4-bit skill storage)
- **Batch Processing**: Process entities in chunks (default: 128)
- **SIMD**: Enable with `-DENABLE_SIMD=ON`
- **Profiling**: Enable with `-DENABLE_PROFILING=ON`

## Next Steps

1. Implement source files (`.cpp`) for all headers
2. Add unit tests
3. Implement rendering system
4. Add save/load functionality
5. Optimize hot paths based on profiling

## Documentation

- `DESIGN_DOCUMENT.md`: Complete game design
- `PROJECT_STRUCTURE.md`: Detailed class and method reference
- `PLATFORM_ABSTRACTION.md`: Platform abstraction layer documentation
- `README.md`: General project information
