#pragma once

#include "Platform/IInput.h"
#include <SDL2/SDL.h>
#include <unordered_map>
#include <string>

namespace Platform {

// SDL implementation of input interface
class SDLInput : public IInput {
public:
    SDLInput();
    ~SDLInput() override;
    
    // IInput interface
    bool Initialize() override;
    void Shutdown() override;
    void Update() override;
    const InputState& GetState() const override;
    bool IsKeyDown(KeyCode key) const override;
    bool IsKeyPressed(KeyCode key) const override;
    bool IsKeyReleased(KeyCode key) const override;
    bool IsMouseButtonDown(MouseButton button) const override;
    bool IsMouseButtonPressed(MouseButton button) const override;
    bool IsMouseButtonReleased(MouseButton button) const override;
    void GetMousePosition(i32& x, i32& y) const override;
    void GetMouseDelta(i32& dx, i32& dy) const override;
    void GetMouseWheel(i32& x, i32& y) const override;
    bool IsWindowFocused() const override;
    bool IsWindowMinimized() const override;
    bool IsWindowCloseRequested() const override;
    void StartTextInput() override;
    void StopTextInput() override;
    bool IsTextInputActive() const override;
    std::string GetTextInput() const override;
    void ClearTextInput() override;
    std::string GetKeyName(KeyCode key) const override;
    
private:
    InputState current_state_;
    InputState previous_state_;
    bool initialized_ = false;
    
    // SDL key code to platform key code mapping
    std::unordered_map<SDL_Scancode, KeyCode> scancode_to_keycode_;
    std::unordered_map<KeyCode, SDL_Scancode> keycode_to_scancode_;
    
    void InitializeKeyMappings();
    KeyCode SDLScancodeToKeyCode(SDL_Scancode scancode) const;
    SDL_Scancode KeyCodeToSDLScancode(KeyCode keycode) const;
    MouseButton SDLButtonToMouseButton(u8 sdl_button) const;
    void ProcessKeyboardEvent(const SDL_KeyboardEvent& event);
    void ProcessMouseMotionEvent(const SDL_MouseMotionEvent& event);
    void ProcessMouseButtonEvent(const SDL_MouseButtonEvent& event);
    void ProcessMouseWheelEvent(const SDL_MouseWheelEvent& event);
    void ProcessWindowEvent(const SDL_WindowEvent& event);
    void ProcessTextInputEvent(const SDL_TextInputEvent& event);
    void UpdateKeyStates();
};

} // namespace Platform



