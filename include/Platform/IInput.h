#pragma once

#include "Core/Types.h"
#include <vector>
#include <string>

namespace Platform {

// Key codes (platform-agnostic)
enum class KeyCode : u32 {
    Unknown = 0,
    
    // Letters
    A = 4, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num1 = 30, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    
    // Special keys
    Return = 40,
    Escape = 41,
    Backspace = 42,
    Tab = 43,
    Space = 44,
    
    // Arrow keys
    Up = 82,
    Down = 81,
    Left = 80,
    Right = 79,
    
    // Modifiers
    LeftShift = 225,
    RightShift = 229,
    LeftCtrl = 224,
    RightCtrl = 228,
    LeftAlt = 226,
    RightAlt = 230,
    
    // Function keys
    F1 = 58, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
};

// Mouse button codes
enum class MouseButton : u8 {
    Left = 0,
    Right = 1,
    Middle = 2,
    X1 = 3,
    X2 = 4
};

// Input state
struct InputState {
    // Keyboard state
    bool keys[512] = {false};
    bool keys_pressed[512] = {false};  // True only on the frame key was pressed
    bool keys_released[512] = {false}; // True only on the frame key was released
    
    // Mouse state
    i32 mouse_x = 0;
    i32 mouse_y = 0;
    i32 mouse_delta_x = 0;
    i32 mouse_delta_y = 0;
    bool mouse_buttons[5] = {false};
    bool mouse_buttons_pressed[5] = {false};
    bool mouse_buttons_released[5] = {false};
    i32 mouse_wheel_x = 0;
    i32 mouse_wheel_y = 0;
    
    // Window state
    bool window_focused = true;
    bool window_minimized = false;
    bool window_close_requested = false;
    
    // Text input
    std::string text_input;
    bool text_input_active = false;
};

// Input interface - abstract base class
class IInput {
public:
    virtual ~IInput() = default;
    
    // Initialize input system
    virtual bool Initialize() = 0;
    
    // Shutdown input system
    virtual void Shutdown() = 0;
    
    // Update input state (call once per frame)
    virtual void Update() = 0;
    
    // Get current input state
    virtual const InputState& GetState() const = 0;
    
    // Key queries
    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsKeyPressed(KeyCode key) const = 0;  // True only on press frame
    virtual bool IsKeyReleased(KeyCode key) const = 0; // True only on release frame
    
    // Mouse queries
    virtual bool IsMouseButtonDown(MouseButton button) const = 0;
    virtual bool IsMouseButtonPressed(MouseButton button) const = 0;
    virtual bool IsMouseButtonReleased(MouseButton button) const = 0;
    
    // Mouse position
    virtual void GetMousePosition(i32& x, i32& y) const = 0;
    virtual void GetMouseDelta(i32& dx, i32& dy) const = 0;
    
    // Mouse wheel
    virtual void GetMouseWheel(i32& x, i32& y) const = 0;
    
    // Window focus
    virtual bool IsWindowFocused() const = 0;
    virtual bool IsWindowMinimized() const = 0;
    
    // Window close request (from window manager)
    virtual bool IsWindowCloseRequested() const = 0;
    
    // Text input
    virtual void StartTextInput() = 0;
    virtual void StopTextInput() = 0;
    virtual bool IsTextInputActive() const = 0;
    virtual std::string GetTextInput() const = 0;
    
    // Clear text input
    virtual void ClearTextInput() = 0;
    
    // Get key name (for display)
    virtual std::string GetKeyName(KeyCode key) const = 0;
};

} // namespace Platform




