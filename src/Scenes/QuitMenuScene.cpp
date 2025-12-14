#include "Scenes/QuitMenuScene.h"
#include "Scenes/SceneManager.h"
#include "Platform/PlatformManager.h"

namespace Game {

QuitMenuScene::QuitMenuScene()
    : MenuScene("QuitMenu")
{
}

void QuitMenuScene::BuildMenuItems(std::vector<MenuItem>& items) {
    items.clear();
    
    items.push_back({
        "Yes",
        [this]() { OnYesSelected(); },
        true
    });
    
    items.push_back({
        "No",
        [this]() { OnNoSelected(); },
        true
    });
}

std::string QuitMenuScene::GetTitle() const {
    return "Quit Game?";
}

void QuitMenuScene::OnYesSelected() {
    // Request game exit through scene manager
    if (scene_manager_) {
        scene_manager_->RequestExit();
    }
}

void QuitMenuScene::OnNoSelected() {
    // Return to main menu
    if (scene_manager_) {
        scene_manager_->PopScene();
    }
}

} // namespace Game
