# Fantasy Sim - Anime Fantasy World/Life Simulator

A large-scale simulation game featuring 1 million inhabitants across 100 regions, each with 200 skills, races, heroes, and dynamic events.

## Building

### Quick Start (Recommended)

**Windows:**
```batch
scripts\setup_env.bat    # First time only
scripts\build.bat        # Build the project
scripts\run.bat          # Run the executable
```

**Linux/Mac:**
```bash
chmod +x scripts/*.sh    # First time only
./scripts/setup_env.sh   # First time only
./scripts/build.sh       # Build the project
./scripts/run.sh         # Run the executable
```

### Prerequisites
- CMake 3.20 or higher
- C++20 compatible compiler (MSVC 2019+, GCC 10+, Clang 12+)
- Git (for cloning vcpkg)
- vcpkg (automatically set up by setup scripts)

### Manual Setup

See [BUILD_INSTRUCTIONS.md](BUILD_INSTRUCTIONS.md) for detailed manual setup instructions.

## Project Structure

```
fantasy_sim/
├── include/          # Header files
├── src/              # Source files
├── config/           # Configuration files
├── assets/           # Game assets
└── tests/            # Unit tests
```

## Configuration

Edit `config/default.json` to customize simulation parameters. See `DESIGN_DOCUMENT.md` for detailed configuration options.





