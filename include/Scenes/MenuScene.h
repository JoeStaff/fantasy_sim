#pragma once

#include "Scenes/Scene.h"
#include "Core/Types.h"
#include <vector>
#include <string>
#include <functional>

namespace Game {

// Base class for menu-based scenes
// Provides common menu functionality to reduce duplicate code
class MenuScene : public Scene {
public:
    // Menu item structure
    struct MenuItem {
        std::string label;
        std::function<void()> action;
        bool enabled = true;
    };
    
    MenuScene(const std::string& name);
    virtual ~MenuScene() = default;
    
    // Scene interface
    bool Initialize(Platform::PlatformManager* platform_manager) override;
    void Shutdown() override;
    void Update(f32 delta_time) override;
    void Render(Platform::IVideo* video) override;
    void ProcessInput(Platform::IInput* input) override;
    
protected:
    // Override to populate menu items
    virtual void BuildMenuItems(std::vector<MenuItem>& items) = 0;
    
    // Menu configuration (can be overridden)
    virtual i32 GetMenuStartX() const;  // X position for menu
    virtual i32 GetMenuStartY() const;  // Y position for menu
    virtual i32 GetMenuItemHeight() const;  // Height between items
    virtual i32 GetMenuItemSpacing() const;  // Spacing between items
    
    // Colors (can be overridden)
    virtual void GetBackgroundColor(u8& r, u8& g, u8& b, u8& a) const;
    virtual void GetTextColor(u8& r, u8& g, u8& b, u8& a) const;
    virtual void GetSelectedTextColor(u8& r, u8& g, u8& b, u8& a) const;
    virtual void GetDisabledTextColor(u8& r, u8& g, u8& b, u8& a) const;
    
    // Title rendering (can be overridden)
    virtual std::string GetTitle() const = 0;
    virtual i32 GetTitleY() const;
    
    // Helper to render text (simple rectangle-based for now)
    void RenderText(Platform::IVideo* video, const std::string& text, i32 x, i32 y, 
                    u8 r, u8 g, u8 b, u8 a = 255);
    
    // Helper to render menu item
    void RenderMenuItem(Platform::IVideo* video, const MenuItem& item, i32 x, i32 y, 
                       bool is_selected, i32 item_height);
    
    // Get menu item bounds for hit testing
    void GetMenuItemBounds(i32 item_index, i32& x, i32& y, i32& width, i32& height) const;
    
private:
    std::vector<MenuItem> menu_items_;
    i32 selected_index_ = 0;
    i32 hovered_index_ = -1;  // Mouse hover index (-1 = no hover)
    bool menu_built_ = false;
    
    // Input handling state
    bool key_held_ = false;
    f32 key_repeat_timer_ = 0.0f;
    static constexpr f32 KEY_REPEAT_DELAY = 0.3f;  // Initial delay
    static constexpr f32 KEY_REPEAT_RATE = 0.1f;    // Repeat rate
    
    // Mouse input helpers
    i32 GetMenuItemAtPosition(i32 mouse_x, i32 mouse_y) const;
    void UpdateMouseHover(i32 mouse_x, i32 mouse_y);
};

} // namespace Game
