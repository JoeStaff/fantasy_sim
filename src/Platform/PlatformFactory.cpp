#include "Platform/PlatformFactory.h"
#include "Platform/SDLVideo.h"
#include "Platform/SDLInput.h"

namespace Platform {

std::unique_ptr<IVideo> PlatformFactory::CreateVideo(VideoBackend backend) {
    switch (backend) {
        case VideoBackend::SDL2:
            return std::make_unique<SDLVideo>();
        default:
            return std::make_unique<SDLVideo>();  // Default to SDL2
    }
}

std::unique_ptr<IInput> PlatformFactory::CreateInput(InputBackend backend) {
    switch (backend) {
        case InputBackend::SDL2:
            return std::make_unique<SDLInput>();
        default:
            return std::make_unique<SDLInput>();  // Default to SDL2
    }
}

std::unique_ptr<IVideo> PlatformFactory::CreateVideoFromString(const std::string& backend_name) {
    if (backend_name == "sdl2" || backend_name == "SDL2") {
        return CreateVideo(VideoBackend::SDL2);
    }
    // Default to SDL2
    return CreateVideo(VideoBackend::SDL2);
}

std::unique_ptr<IInput> PlatformFactory::CreateInputFromString(const std::string& backend_name) {
    if (backend_name == "sdl2" || backend_name == "SDL2") {
        return CreateInput(InputBackend::SDL2);
    }
    // Default to SDL2
    return CreateInput(InputBackend::SDL2);
}

std::string PlatformFactory::GetDefaultVideoBackendName() {
    return "sdl2";
}

std::string PlatformFactory::GetDefaultInputBackendName() {
    return "sdl2";
}

} // namespace Platform
