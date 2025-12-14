#include "Scenes/Scene.h"

namespace Game {

Scene::Scene(const std::string& name)
    : name_(name)
    , scene_manager_(nullptr)
    , platform_manager_(nullptr)
{
}

} // namespace Game
