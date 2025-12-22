#include "Scenes/Scene.h"
#include "Scenes/SceneManager.h"
#include "Scenes/SceneFrame.h"
#include "Platform/PlatformManager.h"

namespace Game {

Scene::Scene(const std::string& name)
    : name_(name)
    , scene_manager_(nullptr)
    , platform_manager_(nullptr)
{
}

void Scene::GetFrameBounds(i32& x, i32& y, i32& width, i32& height) const {
    if (scene_manager_) {
        SceneFrame* frame = scene_manager_->GetSceneFrame(name_);
        if (frame) {
            x = frame->GetX();
            y = frame->GetY();
            width = frame->GetWidth();
            height = frame->GetHeight();
            return;
        }
    }
    
    // No frame, use window size
    if (platform_manager_ && platform_manager_->GetVideo()) {
        auto* video = platform_manager_->GetVideo();
        x = 0;
        y = 0;
        width = video->GetWindowWidth();
        height = video->GetWindowHeight();
    } else {
        x = y = width = height = 0;
    }
}

} // namespace Game
