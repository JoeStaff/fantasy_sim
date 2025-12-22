#include "Scenes/MainMenuScene.h"
#include "Scenes/SceneManager.h"

namespace Game {

MainMenuScene::MainMenuScene()
    : MenuScene("MainMenu")
{
}

void MainMenuScene::BuildMenuItems(std::vector<MenuItem>& items) {
    items.clear();
    
    items.push_back({
        "Start",
        [this]() { OnStartSelected(); },
        true
    });
    
    items.push_back({
        "Settings",
        [this]() { OnSettingsSelected(); },
        true
    });
    
    items.push_back({
        "Quit",
        [this]() { OnQuitSelected(); },
        true
    });
}

std::string MainMenuScene::GetTitle() const {
    return "Fantasy Sim";
}

void MainMenuScene::OnStartSelected() {
    // Transition to world scene
    if (scene_manager_) {
        scene_manager_->SetGridLayout(4, 1);
        // WorldScene takes 3 columns (cells 0-2)
        scene_manager_->AddSceneFrameGrid("WorldScene", 0, 0, 3, 1);
        // WorldSidebarScene takes 1 column (cell 3)
        scene_manager_->AddSceneFrameGrid("WorldSidebarScene", 3, 0, 1, 1);
    }
}

void MainMenuScene::OnSettingsSelected() {
    // TODO: Transition to settings scene when implemented
    // For now, do nothing
}

void MainMenuScene::OnQuitSelected() {
    // Transition to quit confirmation menu
    if (scene_manager_) {
        scene_manager_->PushScene("QuitMenu");
    }
}

} // namespace Game
