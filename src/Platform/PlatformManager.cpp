#include "Platform/PlatformManager.h"
#include "Platform/PlatformFactory.h"

namespace Platform {

PlatformManager::PlatformManager() = default;

PlatformManager::~PlatformManager() {
    Shutdown();
}

bool PlatformManager::Initialize() {
    if (initialized_) {
        return true;
    }
    
    // Create default implementations if not set
    if (!video_ || !input_) {
        CreateDefaultImplementations();
    }
    
    // Initialize video
    if (!video_->Initialize()) {
        return false;
    }
    
    // Initialize input
    if (!input_->Initialize()) {
        video_->Shutdown();
        return false;
    }
    
    initialized_ = true;
    return true;
}

void PlatformManager::Shutdown() {
    if (input_) {
        input_->Shutdown();
    }
    
    if (video_) {
        video_->Shutdown();
    }
    
    initialized_ = false;
}

bool PlatformManager::CreateWindow(const std::string& title, i32 width, i32 height, bool fullscreen) {
    if (!video_) {
        return false;
    }
    
    return video_->CreateWindow(title, width, height, fullscreen);
}

void PlatformManager::SetVideo(std::unique_ptr<IVideo> video) {
    if (initialized_ && video_) {
        video_->Shutdown();
    }
    video_ = std::move(video);
    if (initialized_ && video_) {
        video_->Initialize();
    }
}

void PlatformManager::SetInput(std::unique_ptr<IInput> input) {
    if (initialized_ && input_) {
        input_->Shutdown();
    }
    input_ = std::move(input);
    if (initialized_ && input_) {
        input_->Initialize();
    }
}

void PlatformManager::UpdateInput() {
    if (input_) {
        input_->Update();
    }
}

bool PlatformManager::ShouldClose() const {
    if (input_ && input_->IsWindowCloseRequested()) {
        return true;
    }
    if (video_ && video_->ShouldClose()) {
        return true;
    }
    return false;
}

void PlatformManager::CreateDefaultImplementations() {
    if (!video_) {
        video_ = PlatformFactory::CreateVideo(Platform::VideoBackend::SDL2);
    }
    if (!input_) {
        input_ = PlatformFactory::CreateInput(Platform::InputBackend::SDL2);
    }
}

} // namespace Platform



