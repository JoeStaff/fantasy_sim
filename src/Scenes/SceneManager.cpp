#include "Scenes/SceneManager.h"
#include <iostream>

namespace Game {

SceneManager::SceneManager()
    : initialized_(false)
    , exit_requested_(false)
    , platform_manager_(nullptr)
{
}

SceneManager::~SceneManager() {
    Shutdown();
}

bool SceneManager::Initialize(Platform::PlatformManager* platform_manager) {
    if (initialized_) {
        return true;
    }
    
    if (!platform_manager) {
        std::cerr << "SceneManager: Platform manager is null" << std::endl;
        return false;
    }
    
    platform_manager_ = platform_manager;
    initialized_ = true;
    
    return true;
}

void SceneManager::Shutdown() {
    // Exit all scenes
    while (!scene_stack_.empty()) {
        Scene* scene = scene_stack_.top();
        if (scene) {
            scene->OnExit();
            scene->Shutdown();
        }
        scene_stack_.pop();
    }
    
    // Clear all registered scenes
    scenes_.clear();
    initialized_ = false;
    platform_manager_ = nullptr;
}

void SceneManager::RegisterScene(std::unique_ptr<Scene> scene) {
    if (!scene) {
        std::cerr << "SceneManager: Attempted to register null scene" << std::endl;
        return;
    }
    
    std::string name = scene->GetName();
    if (scenes_.find(name) != scenes_.end()) {
        std::cerr << "SceneManager: Scene '" << name << "' already registered" << std::endl;
        return;
    }
    
    // Set scene manager reference
    scene->SetSceneManager(this);
    
    // Initialize scene
    if (!scene->Initialize(platform_manager_)) {
        std::cerr << "SceneManager: Failed to initialize scene '" << name << "'" << std::endl;
        return;
    }
    
    scenes_[name] = std::move(scene);
}

bool SceneManager::ChangeScene(const std::string& scene_name) {
    Scene* new_scene = FindScene(scene_name);
    if (!new_scene) {
        std::cerr << "SceneManager: Scene '" << scene_name << "' not found" << std::endl;
        return false;
    }
    
    // Exit current scene
    if (!scene_stack_.empty()) {
        Scene* current = scene_stack_.top();
        if (current) {
            current->OnExit();
        }
        scene_stack_.pop();
    }
    
    // Enter new scene
    scene_stack_.push(new_scene);
    new_scene->OnEnter();
    
    return true;
}

bool SceneManager::PushScene(const std::string& scene_name) {
    Scene* new_scene = FindScene(scene_name);
    if (!new_scene) {
        std::cerr << "SceneManager: Scene '" << scene_name << "' not found" << std::endl;
        return false;
    }
    
    // Pause current scene (don't exit, just push new one)
    // Enter new scene
    scene_stack_.push(new_scene);
    new_scene->OnEnter();
    
    return true;
}

bool SceneManager::PopScene() {
    if (scene_stack_.empty()) {
        std::cerr << "SceneManager: No scene to pop" << std::endl;
        return false;
    }
    
    // Exit current scene
    Scene* current = scene_stack_.top();
    if (current) {
        current->OnExit();
    }
    scene_stack_.pop();
    
    // Resume previous scene (if any)
    if (!scene_stack_.empty()) {
        Scene* previous = scene_stack_.top();
        if (previous) {
            previous->OnEnter();
        }
    }
    
    return true;
}

Scene* SceneManager::GetCurrentScene() const {
    if (scene_stack_.empty()) {
        return nullptr;
    }
    return scene_stack_.top();
}

void SceneManager::Update(f32 delta_time) {
    Scene* current = GetCurrentScene();
    if (current) {
        current->Update(delta_time);
    }
}

void SceneManager::Render() {
    Scene* current = GetCurrentScene();
    if (current && platform_manager_ && platform_manager_->GetVideo()) {
        current->Render(platform_manager_->GetVideo());
    }
}

void SceneManager::ProcessInput() {
    Scene* current = GetCurrentScene();
    if (current && platform_manager_ && platform_manager_->GetInput()) {
        current->ProcessInput(platform_manager_->GetInput());
    }
}

Scene* SceneManager::FindScene(const std::string& scene_name) const {
    auto it = scenes_.find(scene_name);
    if (it != scenes_.end()) {
        return it->second.get();
    }
    return nullptr;
}

} // namespace Game
