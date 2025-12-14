#include "Platform/SDLVideo.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

namespace Platform {

SDLVideo::SDLVideo() = default;

SDLVideo::~SDLVideo() {
    Shutdown();
}

bool SDLVideo::Initialize() {
    if (initialized_) {
        return true;
    }
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }
    
    if (!InitializeTTF()) {
        std::cerr << "Warning: Failed to initialize SDL_ttf" << std::endl;
        // Continue anyway, font rendering just won't work
    }
    
    initialized_ = true;
    return true;
}

void SDLVideo::Shutdown() {
    DestroyWindow();
    ShutdownTTF();
    
    if (initialized_) {
        SDL_Quit();
        initialized_ = false;
    }
}

bool SDLVideo::CreateWindow(const std::string& title, i32 width, i32 height, bool fullscreen) {
    if (window_) {
        DestroyWindow();
    }
    
    window_width_ = width;
    window_height_ = height;
    
    u32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        flags
    );
    
    if (!window_) {
        return false;
    }
    
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    
    window_handle_.ptr = window_;
    renderer_handle_.ptr = renderer_;
    
    return true;
}

void SDLVideo::DestroyWindow() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    window_handle_.ptr = nullptr;
    renderer_handle_.ptr = nullptr;
}

WindowHandle* SDLVideo::GetWindowHandle() const {
    return const_cast<WindowHandle*>(&window_handle_);
}

RendererHandle* SDLVideo::GetRendererHandle() const {
    return const_cast<RendererHandle*>(&renderer_handle_);
}

void SDLVideo::BeginFrame() {
    // Events are processed by input system, not here
    // This prevents double-processing of events
}

void SDLVideo::EndFrame() {
    if (renderer_) {
        SDL_RenderPresent(renderer_);
    }
}

void SDLVideo::Clear(u8 r, u8 g, u8 b, u8 a) {
    if (renderer_) {
        SDL_SetRenderDrawColor(renderer_, r, g, b, a);
        SDL_RenderClear(renderer_);
    }
}

void SDLVideo::SetDrawColor(u8 r, u8 g, u8 b, u8 a) {
    if (renderer_) {
        SDL_SetRenderDrawColor(renderer_, r, g, b, a);
    }
}

void SDLVideo::DrawRect(i32 x, i32 y, i32 w, i32 h) {
    if (renderer_) {
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderFillRect(renderer_, &rect);
    }
}

void SDLVideo::DrawRectOutline(i32 x, i32 y, i32 w, i32 h) {
    if (renderer_) {
        SDL_Rect rect = {x, y, w, h};
        SDL_RenderDrawRect(renderer_, &rect);
    }
}

void SDLVideo::DrawLine(i32 x1, i32 y1, i32 x2, i32 y2) {
    if (renderer_) {
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }
}

void SDLVideo::DrawPoint(i32 x, i32 y) {
    if (renderer_) {
        SDL_RenderDrawPoint(renderer_, x, y);
    }
}

i32 SDLVideo::GetWindowWidth() const {
    // Always query actual window size to handle resizing
    if (window_) {
        i32 width, height;
        SDL_GetWindowSize(window_, &width, &height);
        return width;
    }
    return window_width_;
}

i32 SDLVideo::GetWindowHeight() const {
    // Always query actual window size to handle resizing
    if (window_) {
        i32 width, height;
        SDL_GetWindowSize(window_, &width, &height);
        return height;
    }
    return window_height_;
}

bool SDLVideo::ShouldClose() const {
    // Window close is now handled by input system
    // This method is kept for compatibility but always returns false
    return false;
}

void SDLVideo::SetWindowTitle(const std::string& title) {
    if (window_) {
        SDL_SetWindowTitle(window_, title.c_str());
    }
}

void SDLVideo::SetFullscreen(bool fullscreen) {
    if (window_) {
        SDL_SetWindowFullscreen(window_, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    }
}

void SDLVideo::GetWindowSize(i32& width, i32& height) const {
    width = window_width_;
    height = window_height_;
    
    if (window_) {
        SDL_GetWindowSize(window_, &width, &height);
    }
}

void SDLVideo::SetViewport(i32 x, i32 y, i32 w, i32 h) {
    if (renderer_) {
        SDL_Rect viewport = {x, y, w, h};
        SDL_RenderSetViewport(renderer_, &viewport);
    }
}

void SDLVideo::ResetViewport() {
    if (renderer_) {
        SDL_RenderSetViewport(renderer_, nullptr);
    }
}

bool SDLVideo::InitializeTTF() {
    if (ttf_initialized_) {
        return true;
    }
    
    if (TTF_Init() < 0) {
        std::cerr << "TTF_Init error: " << TTF_GetError() << std::endl;
        return false;
    }
    
    ttf_initialized_ = true;
    return true;
}

void SDLVideo::ShutdownTTF() {
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    
    if (ttf_initialized_) {
        TTF_Quit();
        ttf_initialized_ = false;
    }
}

bool SDLVideo::LoadFont(const std::string& font_path, i32 size) {
    if (!ttf_initialized_) {
        if (!InitializeTTF()) {
            return false;
        }
    }
    
    // Close existing font if any
    if (font_) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    
    font_ = TTF_OpenFont(font_path.c_str(), size);
    if (!font_) {
        std::cerr << "Failed to load font '" << font_path << "': " << TTF_GetError() << std::endl;
        return false;
    }
    
    font_size_ = size;
    return true;
}

void SDLVideo::SetFontSize(i32 size) {
    if (font_ && size > 0) {
        // Need to reload font with new size
        // Get current font path (we don't store it, so this is a limitation)
        // For now, just update the size variable
        // In a full implementation, we'd store the font path
        font_size_ = size;
    }
}

void SDLVideo::DrawText(const std::string& text, i32 x, i32 y, u8 r, u8 g, u8 b, u8 a) {
    if (!font_ || !renderer_ || text.empty()) {
        return;
    }
    
    SDL_Color color = {r, g, b, a};
    SDL_Surface* surface = TTF_RenderText_Blended(font_, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect dest_rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer_, texture, nullptr, &dest_rect);
    
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void SDLVideo::GetTextSize(const std::string& text, i32& width, i32& height) {
    width = 0;
    height = 0;
    
    if (!font_ || text.empty()) {
        return;
    }
    
    i32 w, h;
    if (TTF_SizeText(font_, text.c_str(), &w, &h) == 0) {
        width = w;
        height = h;
    }
}

} // namespace Platform
