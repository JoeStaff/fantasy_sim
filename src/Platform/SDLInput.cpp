#include "Platform/SDLInput.h"
#include <SDL2/SDL.h>

namespace Platform {

SDLInput::SDLInput() = default;

SDLInput::~SDLInput() {
    Shutdown();
}

bool SDLInput::Initialize() {
    if (initialized_) {
        return true;
    }
    
    InitializeKeyMappings();
    initialized_ = true;
    return true;
}

void SDLInput::Shutdown() {
    if (initialized_) {
        StopTextInput();
        initialized_ = false;
    }
}

void SDLInput::Update() {
    // Store previous state
    previous_state_ = current_state_;
    
    // Reset frame-specific states
    for (size_t i = 0; i < 512; ++i) {
        current_state_.keys_pressed[i] = false;
        current_state_.keys_released[i] = false;
    }
    for (size_t i = 0; i < 5; ++i) {
        current_state_.mouse_buttons_pressed[i] = false;
        current_state_.mouse_buttons_released[i] = false;
    }
    
    current_state_.mouse_delta_x = 0;
    current_state_.mouse_delta_y = 0;
    current_state_.mouse_wheel_x = 0;
    current_state_.mouse_wheel_y = 0;
    current_state_.text_input.clear();
    current_state_.window_close_requested = false;
    
    // Process SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                current_state_.window_close_requested = true;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                ProcessKeyboardEvent(event.key);
                break;
            case SDL_MOUSEMOTION:
                ProcessMouseMotionEvent(event.motion);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                ProcessMouseButtonEvent(event.button);
                break;
            case SDL_MOUSEWHEEL:
                ProcessMouseWheelEvent(event.wheel);
                break;
            case SDL_WINDOWEVENT:
                ProcessWindowEvent(event.window);
                break;
            case SDL_TEXTINPUT:
                ProcessTextInputEvent(event.text);
                break;
        }
    }
    
    UpdateKeyStates();
}

const InputState& SDLInput::GetState() const {
    return current_state_;
}

bool SDLInput::IsKeyDown(KeyCode key) const {
    u32 index = static_cast<u32>(key);
    if (index >= 512) return false;
    return current_state_.keys[index];
}

bool SDLInput::IsKeyPressed(KeyCode key) const {
    u32 index = static_cast<u32>(key);
    if (index >= 512) return false;
    return current_state_.keys_pressed[index];
}

bool SDLInput::IsKeyReleased(KeyCode key) const {
    u32 index = static_cast<u32>(key);
    if (index >= 512) return false;
    return current_state_.keys_released[index];
}

bool SDLInput::IsMouseButtonDown(MouseButton button) const {
    u8 index = static_cast<u8>(button);
    if (index >= 5) return false;
    return current_state_.mouse_buttons[index];
}

bool SDLInput::IsMouseButtonPressed(MouseButton button) const {
    u8 index = static_cast<u8>(button);
    if (index >= 5) return false;
    return current_state_.mouse_buttons_pressed[index];
}

bool SDLInput::IsMouseButtonReleased(MouseButton button) const {
    u8 index = static_cast<u8>(button);
    if (index >= 5) return false;
    return current_state_.mouse_buttons_released[index];
}

void SDLInput::GetMousePosition(i32& x, i32& y) const {
    x = current_state_.mouse_x;
    y = current_state_.mouse_y;
}

void SDLInput::GetMouseDelta(i32& dx, i32& dy) const {
    dx = current_state_.mouse_delta_x;
    dy = current_state_.mouse_delta_y;
}

void SDLInput::GetMouseWheel(i32& x, i32& y) const {
    x = current_state_.mouse_wheel_x;
    y = current_state_.mouse_wheel_y;
}

bool SDLInput::IsWindowFocused() const {
    return current_state_.window_focused;
}

bool SDLInput::IsWindowMinimized() const {
    return current_state_.window_minimized;
}

bool SDLInput::IsWindowCloseRequested() const {
    return current_state_.window_close_requested;
}

void SDLInput::StartTextInput() {
    if (!current_state_.text_input_active) {
        SDL_StartTextInput();
        current_state_.text_input_active = true;
    }
}

void SDLInput::StopTextInput() {
    if (current_state_.text_input_active) {
        SDL_StopTextInput();
        current_state_.text_input_active = false;
    }
}

bool SDLInput::IsTextInputActive() const {
    return current_state_.text_input_active;
}

std::string SDLInput::GetTextInput() const {
    return current_state_.text_input;
}

void SDLInput::ClearTextInput() {
    current_state_.text_input.clear();
}

std::string SDLInput::GetKeyName(KeyCode key) const {
    SDL_Scancode scancode = KeyCodeToSDLScancode(key);
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return "Unknown";
    }
    return SDL_GetScancodeName(scancode);
}

void SDLInput::InitializeKeyMappings() {
    // Map SDL scancodes to our KeyCode enum
    // This is a simplified mapping - you may want to expand this
    scancode_to_keycode_[SDL_SCANCODE_A] = KeyCode::A;
    scancode_to_keycode_[SDL_SCANCODE_B] = KeyCode::B;
    scancode_to_keycode_[SDL_SCANCODE_C] = KeyCode::C;
    scancode_to_keycode_[SDL_SCANCODE_D] = KeyCode::D;
    scancode_to_keycode_[SDL_SCANCODE_E] = KeyCode::E;
    scancode_to_keycode_[SDL_SCANCODE_F] = KeyCode::F;
    scancode_to_keycode_[SDL_SCANCODE_G] = KeyCode::G;
    scancode_to_keycode_[SDL_SCANCODE_H] = KeyCode::H;
    scancode_to_keycode_[SDL_SCANCODE_I] = KeyCode::I;
    scancode_to_keycode_[SDL_SCANCODE_J] = KeyCode::J;
    scancode_to_keycode_[SDL_SCANCODE_K] = KeyCode::K;
    scancode_to_keycode_[SDL_SCANCODE_L] = KeyCode::L;
    scancode_to_keycode_[SDL_SCANCODE_M] = KeyCode::M;
    scancode_to_keycode_[SDL_SCANCODE_N] = KeyCode::N;
    scancode_to_keycode_[SDL_SCANCODE_O] = KeyCode::O;
    scancode_to_keycode_[SDL_SCANCODE_P] = KeyCode::P;
    scancode_to_keycode_[SDL_SCANCODE_Q] = KeyCode::Q;
    scancode_to_keycode_[SDL_SCANCODE_R] = KeyCode::R;
    scancode_to_keycode_[SDL_SCANCODE_S] = KeyCode::S;
    scancode_to_keycode_[SDL_SCANCODE_T] = KeyCode::T;
    scancode_to_keycode_[SDL_SCANCODE_U] = KeyCode::U;
    scancode_to_keycode_[SDL_SCANCODE_V] = KeyCode::V;
    scancode_to_keycode_[SDL_SCANCODE_W] = KeyCode::W;
    scancode_to_keycode_[SDL_SCANCODE_X] = KeyCode::X;
    scancode_to_keycode_[SDL_SCANCODE_Y] = KeyCode::Y;
    scancode_to_keycode_[SDL_SCANCODE_Z] = KeyCode::Z;
    
    scancode_to_keycode_[SDL_SCANCODE_1] = KeyCode::Num1;
    scancode_to_keycode_[SDL_SCANCODE_2] = KeyCode::Num2;
    scancode_to_keycode_[SDL_SCANCODE_3] = KeyCode::Num3;
    scancode_to_keycode_[SDL_SCANCODE_4] = KeyCode::Num4;
    scancode_to_keycode_[SDL_SCANCODE_5] = KeyCode::Num5;
    scancode_to_keycode_[SDL_SCANCODE_6] = KeyCode::Num6;
    scancode_to_keycode_[SDL_SCANCODE_7] = KeyCode::Num7;
    scancode_to_keycode_[SDL_SCANCODE_8] = KeyCode::Num8;
    scancode_to_keycode_[SDL_SCANCODE_9] = KeyCode::Num9;
    scancode_to_keycode_[SDL_SCANCODE_0] = KeyCode::Num0;
    
    scancode_to_keycode_[SDL_SCANCODE_RETURN] = KeyCode::Return;
    scancode_to_keycode_[SDL_SCANCODE_ESCAPE] = KeyCode::Escape;
    scancode_to_keycode_[SDL_SCANCODE_BACKSPACE] = KeyCode::Backspace;
    scancode_to_keycode_[SDL_SCANCODE_TAB] = KeyCode::Tab;
    scancode_to_keycode_[SDL_SCANCODE_SPACE] = KeyCode::Space;
    
    scancode_to_keycode_[SDL_SCANCODE_UP] = KeyCode::Up;
    scancode_to_keycode_[SDL_SCANCODE_DOWN] = KeyCode::Down;
    scancode_to_keycode_[SDL_SCANCODE_LEFT] = KeyCode::Left;
    scancode_to_keycode_[SDL_SCANCODE_RIGHT] = KeyCode::Right;
    
    scancode_to_keycode_[SDL_SCANCODE_LSHIFT] = KeyCode::LeftShift;
    scancode_to_keycode_[SDL_SCANCODE_RSHIFT] = KeyCode::RightShift;
    scancode_to_keycode_[SDL_SCANCODE_LCTRL] = KeyCode::LeftCtrl;
    scancode_to_keycode_[SDL_SCANCODE_RCTRL] = KeyCode::RightCtrl;
    scancode_to_keycode_[SDL_SCANCODE_LALT] = KeyCode::LeftAlt;
    scancode_to_keycode_[SDL_SCANCODE_RALT] = KeyCode::RightAlt;
    
    scancode_to_keycode_[SDL_SCANCODE_F1] = KeyCode::F1;
    scancode_to_keycode_[SDL_SCANCODE_F2] = KeyCode::F2;
    scancode_to_keycode_[SDL_SCANCODE_F3] = KeyCode::F3;
    scancode_to_keycode_[SDL_SCANCODE_F4] = KeyCode::F4;
    scancode_to_keycode_[SDL_SCANCODE_F5] = KeyCode::F5;
    scancode_to_keycode_[SDL_SCANCODE_F6] = KeyCode::F6;
    scancode_to_keycode_[SDL_SCANCODE_F7] = KeyCode::F7;
    scancode_to_keycode_[SDL_SCANCODE_F8] = KeyCode::F8;
    scancode_to_keycode_[SDL_SCANCODE_F9] = KeyCode::F9;
    scancode_to_keycode_[SDL_SCANCODE_F10] = KeyCode::F10;
    scancode_to_keycode_[SDL_SCANCODE_F11] = KeyCode::F11;
    scancode_to_keycode_[SDL_SCANCODE_F12] = KeyCode::F12;
    
    // Build reverse mapping
    for (const auto& pair : scancode_to_keycode_) {
        keycode_to_scancode_[pair.second] = pair.first;
    }
}

KeyCode SDLInput::SDLScancodeToKeyCode(SDL_Scancode scancode) const {
    auto it = scancode_to_keycode_.find(scancode);
    if (it != scancode_to_keycode_.end()) {
        return it->second;
    }
    return KeyCode::Unknown;
}

SDL_Scancode SDLInput::KeyCodeToSDLScancode(KeyCode keycode) const {
    auto it = keycode_to_scancode_.find(keycode);
    if (it != keycode_to_scancode_.end()) {
        return it->second;
    }
    return SDL_SCANCODE_UNKNOWN;
}

MouseButton SDLInput::SDLButtonToMouseButton(u8 sdl_button) const {
    switch (sdl_button) {
        case SDL_BUTTON_LEFT: return MouseButton::Left;
        case SDL_BUTTON_RIGHT: return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_X1: return MouseButton::X1;
        case SDL_BUTTON_X2: return MouseButton::X2;
        default: return MouseButton::Left;
    }
}

void SDLInput::ProcessKeyboardEvent(const SDL_KeyboardEvent& event) {
    KeyCode key = SDLScancodeToKeyCode(event.keysym.scancode);
    u32 index = static_cast<u32>(key);
    
    if (index >= 512) return;
    
    if (event.type == SDL_KEYDOWN) {
        if (!previous_state_.keys[index]) {
            current_state_.keys_pressed[index] = true;
        }
        current_state_.keys[index] = true;
    } else if (event.type == SDL_KEYUP) {
        if (previous_state_.keys[index]) {
            current_state_.keys_released[index] = true;
        }
        current_state_.keys[index] = false;
    }
}

void SDLInput::ProcessMouseMotionEvent(const SDL_MouseMotionEvent& event) {
    current_state_.mouse_delta_x = event.xrel;
    current_state_.mouse_delta_y = event.yrel;
    current_state_.mouse_x = event.x;
    current_state_.mouse_y = event.y;
}

void SDLInput::ProcessMouseButtonEvent(const SDL_MouseButtonEvent& event) {
    MouseButton button = SDLButtonToMouseButton(event.button);
    u8 index = static_cast<u8>(button);
    
    if (index >= 5) return;
    
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (!previous_state_.mouse_buttons[index]) {
            current_state_.mouse_buttons_pressed[index] = true;
        }
        current_state_.mouse_buttons[index] = true;
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (previous_state_.mouse_buttons[index]) {
            current_state_.mouse_buttons_released[index] = true;
        }
        current_state_.mouse_buttons[index] = false;
    }
}

void SDLInput::ProcessMouseWheelEvent(const SDL_MouseWheelEvent& event) {
    current_state_.mouse_wheel_x = event.x;
    current_state_.mouse_wheel_y = event.y;
}

void SDLInput::ProcessWindowEvent(const SDL_WindowEvent& event) {
    switch (event.event) {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            current_state_.window_focused = true;
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            current_state_.window_focused = false;
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            current_state_.window_minimized = true;
            break;
        case SDL_WINDOWEVENT_RESTORED:
            current_state_.window_minimized = false;
            break;
    }
}

void SDLInput::ProcessTextInputEvent(const SDL_TextInputEvent& event) {
    if (current_state_.text_input_active) {
        current_state_.text_input += event.text;
    }
}

void SDLInput::UpdateKeyStates() {
    // This is handled in ProcessKeyboardEvent, but kept for consistency
}

} // namespace Platform





