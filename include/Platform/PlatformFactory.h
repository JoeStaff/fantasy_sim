#pragma once

#include "Platform/IVideo.h"
#include "Platform/IInput.h"
#include <memory>
#include <string>

namespace Platform {

// Platform backend types
enum class VideoBackend {
    SDL2,
    // Future: OpenGL, DirectX, Vulkan, etc.
};

enum class InputBackend {
    SDL2,
    // Future: GLFW, Win32, etc.
};

// Factory for creating platform implementations
class PlatformFactory {
public:
    // Create video implementation
    static std::unique_ptr<IVideo> CreateVideo(VideoBackend backend = VideoBackend::SDL2);
    
    // Create input implementation
    static std::unique_ptr<IInput> CreateInput(InputBackend backend = InputBackend::SDL2);
    
    // Create video from string (for config)
    static std::unique_ptr<IVideo> CreateVideoFromString(const std::string& backend_name);
    
    // Create input from string (for config)
    static std::unique_ptr<IInput> CreateInputFromString(const std::string& backend_name);
    
    // Get default backend names
    static std::string GetDefaultVideoBackendName();
    static std::string GetDefaultInputBackendName();
};

} // namespace Platform



