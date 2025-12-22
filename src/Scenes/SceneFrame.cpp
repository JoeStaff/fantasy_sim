#include "Scenes/SceneFrame.h"
#include "Scenes/Scene.h"

namespace Game {

SceneFrame::SceneFrame(Scene* scene, i32 x, i32 y, i32 width, i32 height)
    : scene_(scene)
    , x_(x)
    , y_(y)
    , width_(width)
    , height_(height)
    , has_focus_(false)
    , visible_(true)
{
}

void SceneFrame::SetBounds(i32 x, i32 y, i32 width, i32 height) {
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
}

bool SceneFrame::ContainsPoint(i32 x, i32 y) const {
    return x >= x_ && x < x_ + width_ && y >= y_ && y < y_ + height_;
}

} // namespace Game

