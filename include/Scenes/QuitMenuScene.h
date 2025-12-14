#pragma once

#include "Scenes/MenuScene.h"

namespace Game {

// Quit confirmation menu scene with Yes/No options
class QuitMenuScene : public MenuScene {
public:
    QuitMenuScene();
    virtual ~QuitMenuScene() = default;
    
protected:
    void BuildMenuItems(std::vector<MenuItem>& items) override;
    std::string GetTitle() const override;
    
private:
    void OnYesSelected();
    void OnNoSelected();
};

} // namespace Game
