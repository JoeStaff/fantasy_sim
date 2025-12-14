#pragma once

#include "Scenes/MenuScene.h"

namespace Game {

// Main menu scene with Start, Settings, Quit options
class MainMenuScene : public MenuScene {
public:
    MainMenuScene();
    virtual ~MainMenuScene() = default;
    
protected:
    void BuildMenuItems(std::vector<MenuItem>& items) override;
    std::string GetTitle() const override;
    
private:
    void OnStartSelected();
    void OnSettingsSelected();
    void OnQuitSelected();
};

} // namespace Game
