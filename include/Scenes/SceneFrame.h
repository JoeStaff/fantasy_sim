#pragma once

#include "Core/Types.h"
#include <string>

namespace Game {

// Forward declaration
class Scene;

// Represents a scene as a frame that can be positioned on screen
// Each frame has bounds, focus state, and can be overlays or grid-aligned
class SceneFrame {
public:
    SceneFrame(Scene* scene, i32 x, i32 y, i32 width, i32 height);
    
    // Get the scene this frame wraps
    Scene* GetScene() const { return scene_; }
    
    // Get frame bounds
    i32 GetX() const { return x_; }
    i32 GetY() const { return y_; }
    i32 GetWidth() const { return width_; }
    i32 GetHeight() const { return height_; }
    
    // Set frame bounds
    void SetBounds(i32 x, i32 y, i32 width, i32 height);
    
    // Check if a point is within this frame
    bool ContainsPoint(i32 x, i32 y) const;
    
    // Focus management
    bool HasFocus() const { return has_focus_; }
    void SetFocus(bool focus) { has_focus_ = focus; }
    
    // Visibility
    bool IsVisible() const { return visible_; }
    void SetVisible(bool visible) { visible_ = visible; }
    
private:
    Scene* scene_ = nullptr;
    i32 x_ = 0;
    i32 y_ = 0;
    i32 width_ = 0;
    i32 height_ = 0;
    bool has_focus_ = false;
    bool visible_ = true;
};

} // namespace Game

