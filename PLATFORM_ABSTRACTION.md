# Platform Abstraction Layer

The Fantasy Sim project uses a platform abstraction layer to separate video rendering and input handling from the game logic. This allows for easy swapping of backends (SDL, OpenGL, DirectX, etc.) without modifying game code.

## Architecture

### Interfaces

#### `Platform::IVideo`
Abstract interface for video/rendering operations. All rendering code should use this interface rather than directly calling platform-specific APIs.

**Key Features**:
- Window management
- Rendering primitives (rectangles, lines, points)
- Viewport control
- Fullscreen support

#### `Platform::IInput`
Abstract interface for input handling. All input code should use this interface.

**Key Features**:
- Keyboard input (key down/pressed/released states)
- Mouse input (position, buttons, wheel)
- Text input support
- Window focus/minimize detection

### Implementations

#### SDL2 Implementation
- `Platform::SDLVideo` - SDL2 video/rendering implementation
- `Platform::SDLInput` - SDL2 input implementation

### Factory Pattern

`Platform::PlatformFactory` provides a factory for creating platform implementations:

```cpp
// Create default SDL2 implementations
auto video = PlatformFactory::CreateVideo(Platform::VideoBackend::SDL2);
auto input = PlatformFactory::CreateInput(Platform::InputBackend::SDL2);

// Or from configuration strings
auto video = PlatformFactory::CreateVideoFromString("sdl2");
auto input = PlatformFactory::CreateInputFromString("sdl2");
```

## Usage

### In Game Class

The `Game` class uses platform interfaces:

```cpp
class Game {
private:
    std::unique_ptr<Platform::IVideo> video_;
    std::unique_ptr<Platform::IInput> input_;
    
    // Can swap implementations
    void SetVideo(std::unique_ptr<Platform::IVideo> video);
    void SetInput(std::unique_ptr<Platform::IInput> input);
};
```

### Initialization

```cpp
bool Game::Initialize() {
    // Create platform implementations
    video_ = PlatformFactory::CreateVideo(Platform::VideoBackend::SDL2);
    input_ = PlatformFactory::CreateInput(Platform::InputBackend::SDL2);
    
    // Initialize
    if (!video_->Initialize()) return false;
    if (!input_->Initialize()) return false;
    
    // Create window
    if (!video_->CreateWindow("Fantasy Sim", 1280, 720)) return false;
    
    return true;
}
```

### Rendering

```cpp
void Game::Render() {
    video_->BeginFrame();
    video_->Clear(0, 0, 0, 255);  // Clear to black
    
    // Draw game elements using platform-agnostic interface
    video_->SetDrawColor(255, 255, 255, 255);
    video_->DrawRect(100, 100, 200, 200);
    
    video_->EndFrame();
}
```

### Input Handling

```cpp
void Game::ProcessInput() {
    input_->Update();
    
    // Check keyboard
    if (input_->IsKeyPressed(Platform::KeyCode::Escape)) {
        is_running_ = false;
    }
    
    if (input_->IsKeyDown(Platform::KeyCode::W)) {
        // Move forward
    }
    
    // Check mouse
    if (input_->IsMouseButtonPressed(Platform::MouseButton::Left)) {
        i32 x, y;
        input_->GetMousePosition(x, y);
        // Handle click
    }
    
    // Get mouse delta for camera
    i32 dx, dy;
    input_->GetMouseDelta(dx, dy);
    // Update camera
}
```

## Adding New Backends

### Adding a New Video Backend

1. Create a new class inheriting from `Platform::IVideo`:

```cpp
class OpenGLVideo : public Platform::IVideo {
public:
    bool Initialize() override;
    void Shutdown() override;
    // ... implement all virtual methods
};
```

2. Add backend type to `VideoBackend` enum in `PlatformFactory.h`

3. Update `PlatformFactory::CreateVideo()` to handle new backend

### Adding a New Input Backend

1. Create a new class inheriting from `Platform::IInput`:

```cpp
class GLFWInput : public Platform::IInput {
public:
    bool Initialize() override;
    void Shutdown() override;
    // ... implement all virtual methods
};
```

2. Add backend type to `InputBackend` enum in `PlatformFactory.h`

3. Update `PlatformFactory::CreateInput()` to handle new backend

## Benefits

1. **Testability**: Can create mock implementations for testing
2. **Flexibility**: Easy to swap backends without changing game code
3. **Platform Support**: Can support multiple platforms with different backends
4. **Future-Proofing**: Easy to add new backends (Vulkan, Metal, etc.)

## Example: Swapping Implementations

```cpp
// Use SDL2
auto game = Game();
game.Initialize();  // Uses SDL2 by default

// Or swap to different backend
auto game = Game();
game.SetVideo(PlatformFactory::CreateVideo(Platform::VideoBackend::OpenGL));
game.SetInput(PlatformFactory::CreateInput(Platform::InputBackend::GLFW));
game.Initialize();
```

## Configuration

Future enhancement: Add platform backend selection to configuration:

```json
{
  "platform": {
    "video_backend": "sdl2",
    "input_backend": "sdl2"
  }
}
```

Then load from config:

```cpp
auto video_backend = config.platform.video_backend;
video_ = PlatformFactory::CreateVideoFromString(video_backend);
```





