#pragma once

#include "Core/Types.h"
#include "Core/Config.h"
#include "Simulation/SimulationManager.h"
#include "ECS/System.h"  // Includes Coordinator
#include "Platform/PlatformManager.h"
#include "Scenes/SceneManager.h"
#include <memory>
#include <string>

namespace Game {

// Main game class
class Game {
public:
    Game();
    ~Game();
    
    // Initialize game
    bool Initialize();
    
    // Run game loop
    void Run();
    
    // Shutdown game
    void Shutdown();
    
    // Load configuration
    bool LoadConfig(const std::string& config_path = "config/default.json");
    
    // Save game
    bool SaveGame(const std::string& save_path);
    
    // Load game
    bool LoadGame(const std::string& save_path);
    
    // Get platform manager (for accessing video/input if needed)
    Platform::PlatformManager* GetPlatformManager() { return platform_manager_.get(); }
    const Platform::PlatformManager* GetPlatformManager() const { return platform_manager_.get(); }
    
    // Get scene manager
    SceneManager* GetSceneManager() { return scene_manager_.get(); }
    const SceneManager* GetSceneManager() const { return scene_manager_.get(); }
    
private:
    // Update game state
    void Update(f32 delta_time);
    
    // Render game
    void Render();
    
    // Handle input
    void ProcessInput();
    
    // Initialize platform systems
    bool InitializePlatform();
    
    // Initialize ECS
    void InitializeECS();
    
    // Initialize systems
    void InitializeSystems();
    
    // Initialize scenes
    void InitializeScenes();
    
    // Platform manager (handles video and input)
    std::unique_ptr<Platform::PlatformManager> platform_manager_;
    
    // Scene manager (handles scene transitions)
    std::unique_ptr<SceneManager> scene_manager_;
    
    // Core systems
    std::unique_ptr<Simulation::SimulationManager> simulation_manager_;
    ECS::Coordinator* ecs_coordinator_ = nullptr;
    
    // Game state
    bool is_running_ = false;
    f32 frame_time_ = 0.0f;
    u64 frame_count_ = 0;
    
    // Configuration
    Config::Configuration* config_ = nullptr;
};

} // namespace Game
