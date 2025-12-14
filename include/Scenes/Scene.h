#pragma once

#include "Core/Types.h"
#include "Platform/PlatformManager.h"
#include <string>
#include <memory>

namespace Game {

// Forward declaration
class SceneManager;

// Base scene class - all scenes inherit from this
class Scene {
public:
    Scene(const std::string& name);
    virtual ~Scene() = default;
    
    // Get scene name
    const std::string& GetName() const { return name_; }
    
    // Lifecycle methods
    virtual bool Initialize(Platform::PlatformManager* platform_manager) = 0;
    virtual void Shutdown() = 0;
    
    // Update and render (called by SceneManager)
    virtual void Update(f32 delta_time) = 0;
    virtual void Render(Platform::IVideo* video) = 0;
    
    // Input handling
    virtual void ProcessInput(Platform::IInput* input) = 0;
    
    // Scene transition callbacks
    virtual void OnEnter() {}
    virtual void OnExit() {}
    
    // Set scene manager reference (for scene transitions)
    void SetSceneManager(SceneManager* manager) { scene_manager_ = manager; }
    
protected:
    std::string name_;
    SceneManager* scene_manager_ = nullptr;
    Platform::PlatformManager* platform_manager_ = nullptr;
};

} // namespace Game
