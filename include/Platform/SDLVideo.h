#pragma once

#include "Platform/IVideo.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <memory>

namespace Platform {

// SDL implementation of video interface
class SDLVideo : public IVideo {
public:
    SDLVideo();
    ~SDLVideo() override;
    
    // IVideo interface
    bool Initialize() override;
    void Shutdown() override;
    bool CreateWindow(const std::string& title, i32 width, i32 height, bool fullscreen = false) override;
    void DestroyWindow() override;
    WindowHandle* GetWindowHandle() const override;
    RendererHandle* GetRendererHandle() const override;
    void BeginFrame() override;
    void EndFrame() override;
    void Clear(u8 r, u8 g, u8 b, u8 a = 255) override;
    void SetDrawColor(u8 r, u8 g, u8 b, u8 a = 255) override;
    void DrawRect(i32 x, i32 y, i32 w, i32 h) override;
    void DrawRectOutline(i32 x, i32 y, i32 w, i32 h) override;
    void DrawLine(i32 x1, i32 y1, i32 x2, i32 y2) override;
    void DrawPoint(i32 x, i32 y) override;
    i32 GetWindowWidth() const override;
    i32 GetWindowHeight() const override;
    bool ShouldClose() const override;
    void SetWindowTitle(const std::string& title) override;
    void SetFullscreen(bool fullscreen) override;
    void GetWindowSize(i32& width, i32& height) const override;
    void SetViewport(i32 x, i32 y, i32 w, i32 h) override;
    void ResetViewport() override;
    
    // SDL-specific accessors
    SDL_Window* GetSDLWindow() const { return window_; }
    SDL_Renderer* GetSDLRenderer() const { return renderer_; }
    
    // Font rendering
    bool LoadFont(const std::string& font_path, i32 size) override;
    void SetFontSize(i32 size) override;
    void DrawText(const std::string& text, i32 x, i32 y, u8 r, u8 g, u8 b, u8 a = 255) override;
    void GetTextSize(const std::string& text, i32& width, i32& height) override;
    
private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    TTF_Font* font_ = nullptr;
    i32 font_size_ = 24;
    i32 window_width_ = 0;
    i32 window_height_ = 0;
    bool should_close_ = false;
    bool initialized_ = false;
    bool ttf_initialized_ = false;
    
    WindowHandle window_handle_;
    RendererHandle renderer_handle_;
    
    // Helper to initialize TTF
    bool InitializeTTF();
    // Helper to cleanup TTF
    void ShutdownTTF();
};

} // namespace Platform
