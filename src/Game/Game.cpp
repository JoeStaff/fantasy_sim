#include <iostream>
#include <chrono>
#include <string>
#include <memory>

#include "Game/Game.h"
#include "Scenes/SceneManager.h"
#include "Scenes/MainMenuScene.h"
#include "Scenes/QuitMenuScene.h"
#include "Scenes/WorldScene.h"
#include "ECS/System.h"

namespace Game {

Game::Game() = default;

Game::~Game() {
    Shutdown();
}

bool Game::Initialize() {
    // Load default configuration
    if (!LoadConfig()) {
        std::cerr << "Failed to load configuration" << std::endl;
        return false;
    }
    
    // Initialize platform systems
    if (!InitializePlatform()) {
        std::cerr << "Failed to initialize platform" << std::endl;
        return false;
    }
    
    // Initialize ECS
    InitializeECS();
    
    // Initialize systems
    InitializeSystems();
    
    // Initialize scenes
    InitializeScenes();
    
    is_running_ = true;
    return true;
}

void Game::Run() {
    if (!is_running_) {
        return;
    }
    
    auto last_time = std::chrono::high_resolution_clock::now();
    
    while (is_running_) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_time);
        f32 delta_time = delta.count() / 1000000.0f;  // Convert to seconds
        last_time = current_time;
        
        // Cap delta time to prevent large jumps
        if (delta_time > 0.1f) {
            delta_time = 0.1f;
        }
        
        ProcessInput();
        Update(delta_time);
        Render();
        
        frame_count_++;
    }
}

void Game::Shutdown() {
    is_running_ = false;
    
    if (platform_manager_) {
        platform_manager_->Shutdown();
        platform_manager_.reset();
    }
}

bool Game::LoadConfig(const std::string& config_path) {
    config_ = &Config::Configuration::GetInstance();
    return config_->LoadFromFile(config_path);
}

bool Game::SaveGame(const std::string& save_path) {
    // TODO: Implement save game functionality
    (void)save_path;
    return false;
}

bool Game::LoadGame(const std::string& save_path) {
    // TODO: Implement load game functionality
    (void)save_path;
    return false;
}


void Game::Update(f32 delta_time) {
    // Update scene manager
    if (scene_manager_) {
        scene_manager_->Update(delta_time);
    }
    
    // TODO: Update game systems (when in game scene)
    (void)delta_time;
}

void Game::Render() {
    if (!platform_manager_ || !platform_manager_->GetVideo()) {
        return;
    }
    
    auto* video = platform_manager_->GetVideo();
    video->BeginFrame();
    
    // Render through scene manager
    if (scene_manager_) {
        scene_manager_->Render();
    } else {
        // Fallback: clear screen if no scene manager
        video->Clear(30, 30, 30, 255);
    }
    
    video->EndFrame();
}

void Game::ProcessInput() {
    if (!platform_manager_) {
        return;
    }
    
    // Update input state
    platform_manager_->UpdateInput();
    
    // Check for window close request (from window manager)
    if (platform_manager_->ShouldClose()) {
        is_running_ = false;
        return;
    }
    
    // Check for exit request from scene manager
    if (scene_manager_ && scene_manager_->IsExitRequested()) {
        is_running_ = false;
        return;
    }
    
    // Process input through scene manager
    if (scene_manager_) {
        scene_manager_->ProcessInput();
    }
    
    // Check for Escape key (only if no scene is handling it)
    // Scenes will handle their own Escape key behavior
}

bool Game::InitializePlatform() {
    // Create platform manager
    platform_manager_ = std::make_unique<Platform::PlatformManager>();
    
    // Initialize platform systems
    if (!platform_manager_->Initialize()) {
        return false;
    }
    
    // Create window
    if (!platform_manager_->CreateWindow("Fantasy Sim", 1280, 720, false)) {
        return false;
    }
    
    // Load default font
    auto* video = platform_manager_->GetVideo();
    if (video) {
        // Try to load the default font (SuperPixel)
        // First try relative to executable, then try absolute path
        const char* font_paths[] = {
            "assets/fonts/SuperPixel-m2L8j.ttf",
            "../assets/fonts/SuperPixel-m2L8j.ttf",
            "../../assets/fonts/SuperPixel-m2L8j.ttf"
        };
        
        bool font_loaded = false;
        for (const char* path : font_paths) {
            (void)path;  // Suppress unused variable warning
            /*if (video->LoadFont(path, 32)) {
                font_loaded = true;
                break;
            }*/
        }
        
        if (!font_loaded) {
            // Try Arial as fallback
            const char* arial_paths[] = {
                "assets/fonts/ARIAL.TTF",
                "../assets/fonts/ARIAL.TTF",
                "../../assets/fonts/ARIAL.TTF"
            };
            
            for (const char* path : arial_paths) {
                if (video->LoadFont(path, 32)) {
                    font_loaded = true;
                    break;
                }
            }
        }
        
        if (!font_loaded) {
            std::cerr << "Warning: Failed to load default font. Text rendering may not work correctly." << std::endl;
        }
    }
    
    return true;
}

void Game::InitializeECS() {
    ecs_coordinator_ = &ECS::Coordinator::GetInstance();
    
    // TODO: Register components and systems
}

void Game::InitializeSystems() {
    // TODO: Initialize game systems
}

void Game::InitializeScenes() {
    // Create scene manager
    scene_manager_ = std::make_unique<SceneManager>();
    
    if (!scene_manager_->Initialize(platform_manager_.get())) {
        std::cerr << "Failed to initialize scene manager" << std::endl;
        return;
    }
    
    // Register scenes
    scene_manager_->RegisterScene(std::make_unique<MainMenuScene>());
    scene_manager_->RegisterScene(std::make_unique<QuitMenuScene>());
    scene_manager_->RegisterScene(std::make_unique<WorldScene>());
    
    // Start with main menu
    scene_manager_->ChangeScene("MainMenu");
}

} // namespace Game
