#pragma once

#include "Platform/IVideo.h"
#include "Platform/IInput.h"
#include "Core/Types.h"
#include <memory>
#include <string>

namespace Platform {

// Platform manager - handles video and input initialization
class PlatformManager {
public:
    PlatformManager();
    ~PlatformManager();
    
    // Initialize platform systems
    bool Initialize();
    
    // Shutdown platform systems
    void Shutdown();
    
    // Create window
    bool CreateWindow(const std::string& title, i32 width, i32 height, bool fullscreen = false);
    
    // Get video interface
    IVideo* GetVideo() { return video_.get(); }
    const IVideo* GetVideo() const { return video_.get(); }
    
    // Get input interface
    IInput* GetInput() { return input_.get(); }
    const IInput* GetInput() const { return input_.get(); }
    
    // Set custom implementations (for testing)
    void SetVideo(std::unique_ptr<IVideo> video);
    void SetInput(std::unique_ptr<IInput> input);
    
    // Check if platform is initialized
    bool IsInitialized() const { return initialized_; }
    
    // Update input (call once per frame)
    void UpdateInput();
    
    // Check if window should close
    bool ShouldClose() const;
    
private:
    std::unique_ptr<IVideo> video_;
    std::unique_ptr<IInput> input_;
    bool initialized_ = false;
    
    void CreateDefaultImplementations();
};

} // namespace Platform



