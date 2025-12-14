#include "Scenes/MenuScene.h"
#include "Platform/IVideo.h"
#include "Platform/IInput.h"
#include <algorithm>

namespace Game {

MenuScene::MenuScene(const std::string& name)
    : Scene(name)
    , selected_index_(0)
    , hovered_index_(-1)
    , menu_built_(false)
    , key_held_(false)
    , key_repeat_timer_(0.0f)
{
}

bool MenuScene::Initialize(Platform::PlatformManager* platform_manager) {
    platform_manager_ = platform_manager;
    return true;
}

void MenuScene::Shutdown() {
    menu_items_.clear();
    menu_built_ = false;
}

void MenuScene::Update(f32 delta_time) {
    // Build menu items on first update if not already built
    if (!menu_built_) {
        BuildMenuItems(menu_items_);
        menu_built_ = true;
        
        // Ensure selected index is valid
        if (!menu_items_.empty()) {
            selected_index_ = 0;
            // Find first enabled item
            for (size_t i = 0; i < menu_items_.size(); ++i) {
                if (menu_items_[i].enabled) {
                    selected_index_ = static_cast<i32>(i);
                    break;
                }
            }
        }
    }
    
    // Update key repeat timer
    if (key_held_) {
        key_repeat_timer_ += delta_time;
    } else {
        key_repeat_timer_ = 0.0f;
    }
}

void MenuScene::Render(Platform::IVideo* video) {
    if (!video || menu_items_.empty()) {
        return;
    }
    
    // Clear with background color
    u8 bg_r, bg_g, bg_b, bg_a;
    GetBackgroundColor(bg_r, bg_g, bg_b, bg_a);
    video->Clear(bg_r, bg_g, bg_b, bg_a);
    
    // Render title
    std::string title = GetTitle();
    if (!title.empty()) {
        i32 title_y = GetTitleY();
        i32 window_width = video->GetWindowWidth();
        
        // Get actual text width for centering
        i32 text_width, text_height;
        video->GetTextSize(title, text_width, text_height);
        i32 title_x = (window_width - text_width) / 2;
        
        u8 text_r, text_g, text_b, text_a;
        GetTextColor(text_r, text_g, text_b, text_a);
        RenderText(video, title, title_x, title_y, text_r, text_g, text_b, text_a);
    }
    
    // Render menu items
    i32 menu_x = GetMenuStartX();
    i32 menu_y = GetMenuStartY();
    i32 item_height = GetMenuItemHeight();
    i32 item_spacing = GetMenuItemSpacing();
    
    u8 text_r, text_g, text_b, text_a;
    GetTextColor(text_r, text_g, text_b, text_a);
    
    u8 selected_r, selected_g, selected_b, selected_a;
    GetSelectedTextColor(selected_r, selected_g, selected_b, selected_a);
    
    u8 disabled_r, disabled_g, disabled_b, disabled_a;
    GetDisabledTextColor(disabled_r, disabled_g, disabled_b, disabled_a);
    
    for (size_t i = 0; i < menu_items_.size(); ++i) {
        // Item is selected if it's the selected index OR if mouse is hovering over it
        bool is_selected = (static_cast<size_t>(selected_index_) == i) || 
                          (static_cast<size_t>(hovered_index_) == i);
        i32 item_y = menu_y + static_cast<i32>(i) * (item_height + item_spacing);
        
        RenderMenuItem(video, menu_items_[i], menu_x, item_y, is_selected, item_height);
    }
}

void MenuScene::ProcessInput(Platform::IInput* input) {
    if (!input || menu_items_.empty()) {
        return;
    }
    
    // Get mouse position
    i32 mouse_x, mouse_y;
    input->GetMousePosition(mouse_x, mouse_y);
    
    // Update mouse hover (primary input method)
    UpdateMouseHover(mouse_x, mouse_y);
    
    // Handle mouse click (primary activation method)
    bool mouse_clicked = input->IsMouseButtonPressed(Platform::MouseButton::Left);
    if (mouse_clicked && hovered_index_ >= 0 && 
        hovered_index_ < static_cast<i32>(menu_items_.size())) {
        const MenuItem& item = menu_items_[static_cast<size_t>(hovered_index_)];
        if (item.enabled && item.action) {
            // Update selected index to match hovered item
            selected_index_ = hovered_index_;
            item.action();
            return;  // Don't process keyboard input this frame
        }
    }
    
    // Keyboard input (fallback/secondary method)
    bool up_pressed = input->IsKeyPressed(Platform::KeyCode::Up);
    bool down_pressed = input->IsKeyPressed(Platform::KeyCode::Down);
    bool enter_pressed = input->IsKeyPressed(Platform::KeyCode::Return) || 
                         input->IsKeyPressed(Platform::KeyCode::Space);
    
    bool up_held = input->IsKeyDown(Platform::KeyCode::Up);
    bool down_held = input->IsKeyDown(Platform::KeyCode::Down);
    
    // Handle key repeat for held keys
    bool should_repeat = false;
    if (up_held || down_held) {
        if (!key_held_) {
            // First press
            key_held_ = true;
            should_repeat = true;
        } else if (key_repeat_timer_ >= KEY_REPEAT_DELAY) {
            // Repeat after delay
            should_repeat = true;
            key_repeat_timer_ = 0.0f; // Reset for next repeat
        }
    } else {
        key_held_ = false;
    }
    
    // Navigate menu with keyboard (only if no mouse hover)
    if (hovered_index_ < 0) {
        if (up_pressed || (should_repeat && up_held)) {
            // Move selection up
            i32 start_index = selected_index_;
            do {
                selected_index_--;
                if (selected_index_ < 0) {
                    selected_index_ = static_cast<i32>(menu_items_.size() - 1);
                }
            } while (!menu_items_[static_cast<size_t>(selected_index_)].enabled && 
                     selected_index_ != start_index);
        } else if (down_pressed || (should_repeat && down_held)) {
            // Move selection down
            i32 start_index = selected_index_;
            do {
                selected_index_++;
                if (selected_index_ >= static_cast<i32>(menu_items_.size())) {
                    selected_index_ = 0;
                }
            } while (!menu_items_[static_cast<size_t>(selected_index_)].enabled && 
                     selected_index_ != start_index);
        }
    }
    
    // Activate selected item with keyboard (Enter/Space)
    if (enter_pressed && selected_index_ >= 0 && 
        selected_index_ < static_cast<i32>(menu_items_.size())) {
        const MenuItem& item = menu_items_[static_cast<size_t>(selected_index_)];
        if (item.enabled && item.action) {
            item.action();
        }
    }
}

i32 MenuScene::GetMenuStartX() const {
    // Center horizontally based on longest menu item
    if (platform_manager_ && platform_manager_->GetVideo() && menu_built_ && !menu_items_.empty()) {
        auto* video = platform_manager_->GetVideo();
        i32 window_width = video->GetWindowWidth();
        
        // Find longest menu item text
        i32 max_width = 0;
        for (const auto& item : menu_items_) {
            i32 text_width, text_height;
            video->GetTextSize(item.label, text_width, text_height);
            if (text_width > max_width) {
                max_width = text_width;
            }
        }
        
        return (window_width - max_width) / 2;
    }
    // Fallback: center with estimated width
    if (platform_manager_ && platform_manager_->GetVideo()) {
        i32 window_width = platform_manager_->GetVideo()->GetWindowWidth();
        return window_width / 2 - 100; // Rough estimate
    }
    return 100;
}

i32 MenuScene::GetMenuStartY() const {
    // Center menu vertically on screen
    if (platform_manager_ && platform_manager_->GetVideo() && menu_built_ && !menu_items_.empty()) {
        auto* video = platform_manager_->GetVideo();
        i32 window_height = video->GetWindowHeight();
        i32 item_height = GetMenuItemHeight();
        i32 item_spacing = GetMenuItemSpacing();
        
        // Calculate total menu height
        i32 total_menu_height = static_cast<i32>(menu_items_.size()) * item_height + 
                                (static_cast<i32>(menu_items_.size()) - 1) * item_spacing;
        
        // Center vertically
        return (window_height - total_menu_height) / 2;
    }
    // Fallback: start below title
    return GetTitleY() + 80;
}

i32 MenuScene::GetMenuItemHeight() const {
    return 30;
}

i32 MenuScene::GetMenuItemSpacing() const {
    return 10;
}

void MenuScene::GetBackgroundColor(u8& r, u8& g, u8& b, u8& a) const {
    r = 30;
    g = 30;
    b = 30;
    a = 255;
}

void MenuScene::GetTextColor(u8& r, u8& g, u8& b, u8& a) const {
    r = 200;
    g = 200;
    b = 200;
    a = 255;
}

void MenuScene::GetSelectedTextColor(u8& r, u8& g, u8& b, u8& a) const {
    r = 255;
    g = 255;
    b = 100;
    a = 255;
}

void MenuScene::GetDisabledTextColor(u8& r, u8& g, u8& b, u8& a) const {
    r = 100;
    g = 100;
    b = 100;
    a = 255;
}

i32 MenuScene::GetTitleY() const {
    // Center title vertically in the top portion of the screen
    if (platform_manager_ && platform_manager_->GetVideo()) {
        i32 window_height = platform_manager_->GetVideo()->GetWindowHeight();
        // Position title in upper third of screen
        return window_height / 6;
    }
    return 100;
}

void MenuScene::RenderText(Platform::IVideo* video, const std::string& text, 
                           i32 x, i32 y, u8 r, u8 g, u8 b, u8 a) {
    // Use font rendering if available
    video->DrawText(text, x, y, r, g, b, a);
}

void MenuScene::RenderMenuItem(Platform::IVideo* video, const MenuItem& item, 
                               i32 x, i32 y, bool is_selected, i32 item_height) {
    u8 r, g, b, a;
    
    if (!item.enabled) {
        GetDisabledTextColor(r, g, b, a);
    } else if (is_selected) {
        GetSelectedTextColor(r, g, b, a);
        // Draw selection indicator (simple rectangle outline) - shows hover/selection
        video->SetDrawColor(r, g, b, a);
        // Calculate item width for better outline
        i32 text_width, text_height;
        video->GetTextSize(item.label, text_width, text_height);
        video->DrawRectOutline(x - 10, y - 2, text_width + 20, item_height + 4);
    } else {
        GetTextColor(r, g, b, a);
    }
    
    // Render item text
    RenderText(video, item.label, x, y, r, g, b, a);
}

void MenuScene::GetMenuItemBounds(i32 item_index, i32& x, i32& y, i32& width, i32& height) const {
    if (item_index < 0 || item_index >= static_cast<i32>(menu_items_.size()) || 
        !platform_manager_ || !platform_manager_->GetVideo()) {
        x = y = width = height = 0;
        return;
    }
    
    auto* video = platform_manager_->GetVideo();
    i32 menu_x = GetMenuStartX();
    i32 menu_y = GetMenuStartY();
    i32 item_height = GetMenuItemHeight();
    i32 item_spacing = GetMenuItemSpacing();
    
    x = menu_x - 10;  // Account for outline padding
    y = menu_y + item_index * (item_height + item_spacing) - 2;
    
    // Get text width for this item
    const MenuItem& item = menu_items_[static_cast<size_t>(item_index)];
    i32 text_width, text_height;
    video->GetTextSize(item.label, text_width, text_height);
    
    width = text_width + 20;  // Padding on both sides
    height = item_height + 4;  // Padding top and bottom
}

i32 MenuScene::GetMenuItemAtPosition(i32 mouse_x, i32 mouse_y) const {
    if (!menu_built_ || menu_items_.empty()) {
        return -1;
    }
    
    for (size_t i = 0; i < menu_items_.size(); ++i) {
        i32 x, y, width, height;
        GetMenuItemBounds(static_cast<i32>(i), x, y, width, height);
        
        if (mouse_x >= x && mouse_x < x + width &&
            mouse_y >= y && mouse_y < y + height) {
            // Check if item is enabled
            if (menu_items_[i].enabled) {
                return static_cast<i32>(i);
            }
        }
    }
    
    return -1;
}

void MenuScene::UpdateMouseHover(i32 mouse_x, i32 mouse_y) {
    hovered_index_ = GetMenuItemAtPosition(mouse_x, mouse_y);
    
    // If mouse is hovering over an item, update selected_index to match
    // This provides visual feedback and allows keyboard Enter to work on hovered item
    if (hovered_index_ >= 0) {
        selected_index_ = hovered_index_;
    }
}

} // namespace Game
