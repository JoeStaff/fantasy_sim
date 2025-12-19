# Architecture Overview

## Platform Separation

The platform layer (video and input) has been separated from the Game class for better architecture and testability.

### Architecture Layers

```
┌─────────────────────────────────────┐
│         Game (Game Logic)           │
│  - Simulation Manager              │
│  - ECS Systems                      │
│  - Game State                       │
└──────────────┬──────────────────────┘
               │
               │ Uses
               ▼
┌─────────────────────────────────────┐
│    PlatformManager (Platform)       │
│  - Video Management                 │
│  - Input Management                 │
│  - Window Management                │
└──────────────┬──────────────────────┘
               │
               │ Manages
               ▼
       ┌───────┴───────┐
       │               │
┌──────▼──────┐  ┌─────▼──────┐
│   IVideo    │  │   IInput   │
│  Interface  │  │  Interface │
└──────┬──────┘  └─────┬──────┘
       │               │
       │ Implements    │ Implements
       ▼               ▼
┌──────────────┐  ┌──────────────┐
│  SDLVideo    │  │  SDLInput    │
│ (SDL2 impl)  │  │ (SDL2 impl)  │
└──────────────┘  └──────────────┘
```

### Key Classes

#### `Platform::PlatformManager`
**Location**: `include/Platform/PlatformManager.h`

Manages video and input systems independently from game logic.

**Responsibilities**:
- Initialize/shutdown platform systems
- Create and manage window
- Provide access to video and input interfaces
- Handle platform-specific initialization

**Methods**:
- `bool Initialize()` - Initialize platform systems
- `void Shutdown()` - Shutdown platform systems
- `bool CreateWindow(...)` - Create window
- `IVideo* GetVideo()` - Get video interface
- `IInput* GetInput()` - Get input interface
- `void UpdateInput()` - Update input state (call per frame)
- `bool ShouldClose()` - Check if window should close

#### `Game::Game`
**Location**: `include/Game/Game.h`

Game logic class, no longer directly manages platform.

**Changes**:
- Removed direct `video_` and `input_` members
- Added `platform_manager_` member
- Accesses video/input through `PlatformManager`
- Focuses on game logic only

**Benefits**:
- Separation of concerns
- Easier testing (can mock PlatformManager)
- Platform can be swapped without changing Game class
- Cleaner architecture

### Usage Example

```cpp
// Game class no longer directly manages platform
Game game;
game.Initialize();  // PlatformManager handles platform setup

// Access platform if needed
auto* platform = game.GetPlatformManager();
auto* video = platform->GetVideo();
auto* input = platform->GetInput();

// Game focuses on game logic
game.Run();  // Game loop handles platform through manager
```

### Testing Benefits

```cpp
// Can create mock platform for testing
class MockVideo : public Platform::IVideo { /* ... */ };
class MockInput : public Platform::IInput { /* ... */ };

auto mock_video = std::make_unique<MockVideo>();
auto mock_input = std::make_unique<MockInput>();

Platform::PlatformManager platform;
platform.SetVideo(std::move(mock_video));
platform.SetInput(std::move(mock_input));

// Test game logic without real platform
```

## File Structure

```
include/
├── Platform/
│   ├── IVideo.h           # Video interface
│   ├── IInput.h           # Input interface
│   ├── PlatformManager.h  # Platform manager (NEW)
│   ├── SDLVideo.h         # SDL video implementation
│   ├── SDLInput.h         # SDL input implementation
│   └── PlatformFactory.h  # Factory for creating implementations
│
└── Game/
    └── Game.h             # Game class (uses PlatformManager)

src/
├── Platform/
│   ├── PlatformManager.cpp # Platform manager implementation (NEW)
│   ├── SDLVideo.cpp
│   ├── SDLInput.cpp
│   └── PlatformFactory.cpp
│
└── Game/
    └── Game.cpp            # Game implementation (uses PlatformManager)
```





