#pragma once

#include "Core/Types.h"
#include "Platform/PlatformManager.h"
#include "Scenes/Scene.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <stack>

namespace Game {

// Manages scene transitions and lifecycle
class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    
    // Initialize with platform manager
    bool Initialize(Platform::PlatformManager* platform_manager);
    
    // Shutdown and cleanup
    void Shutdown();
    
    // Register a scene (takes ownership)
    void RegisterScene(std::unique_ptr<Scene> scene);
    
    // Change to a scene by name
    bool ChangeScene(const std::string& scene_name);
    
    // Push scene onto stack (pause current, enter new)
    bool PushScene(const std::string& scene_name);
    
    // Pop scene from stack (resume previous)
    bool PopScene();
    
    // Get current scene
    Scene* GetCurrentScene() const;
    
    // Update current scene
    void Update(f32 delta_time);
    
    // Render current scene
    void Render();
    
    // Process input for current scene
    void ProcessInput();
    
    // Check if manager is initialized
    bool IsInitialized() const { return initialized_; }
    
    // Request game exit (called by scenes like QuitMenu)
    void RequestExit() { exit_requested_ = true; }
    
    // Check if exit was requested
    bool IsExitRequested() const { return exit_requested_; }
    
private:
    bool initialized_ = false;
    bool exit_requested_ = false;
    Platform::PlatformManager* platform_manager_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<Scene>> scenes_;
    std::stack<Scene*> scene_stack_;
    
    // Helper to find scene
    Scene* FindScene(const std::string& scene_name) const;
};

} // namespace Game
