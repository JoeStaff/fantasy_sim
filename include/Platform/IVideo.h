#pragma once

#include "Core/Types.h"
#include <string>
#include <memory>

namespace Platform {

// Forward declarations
struct WindowHandle;
struct RendererHandle;

// Video/rendering interface - abstract base class
class IVideo {
public:
    virtual ~IVideo() = default;
    
    // Initialize video system
    virtual bool Initialize() = 0;
    
    // Shutdown video system
    virtual void Shutdown() = 0;
    
    // Create window
    virtual bool CreateWindow(const std::string& title, i32 width, i32 height, bool fullscreen = false) = 0;
    
    // Destroy window
    virtual void DestroyWindow() = 0;
    
    // Get window handle (platform-specific)
    virtual WindowHandle* GetWindowHandle() const = 0;
    
    // Get renderer handle (platform-specific)
    virtual RendererHandle* GetRendererHandle() const = 0;
    
    // Begin frame rendering
    virtual void BeginFrame() = 0;
    
    // End frame rendering (present)
    virtual void EndFrame() = 0;
    
    // Clear screen
    virtual void Clear(u8 r, u8 g, u8 b, u8 a = 255) = 0;
    
    // Set draw color
    virtual void SetDrawColor(u8 r, u8 g, u8 b, u8 a = 255) = 0;
    
    // Draw rectangle (filled)
    virtual void DrawRect(i32 x, i32 y, i32 w, i32 h) = 0;
    
    // Draw rectangle (outline)
    virtual void DrawRectOutline(i32 x, i32 y, i32 w, i32 h) = 0;
    
    // Draw line
    virtual void DrawLine(i32 x1, i32 y1, i32 x2, i32 y2) = 0;
    
    // Draw point
    virtual void DrawPoint(i32 x, i32 y) = 0;
    
    // Get window width
    virtual i32 GetWindowWidth() const = 0;
    
    // Get window height
    virtual i32 GetWindowHeight() const = 0;
    
    // Check if window should close
    virtual bool ShouldClose() const = 0;
    
    // Set window title
    virtual void SetWindowTitle(const std::string& title) = 0;
    
    // Toggle fullscreen
    virtual void SetFullscreen(bool fullscreen) = 0;
    
    // Get window size
    virtual void GetWindowSize(i32& width, i32& height) const = 0;
    
    // Set viewport
    virtual void SetViewport(i32 x, i32 y, i32 w, i32 h) = 0;
    
    // Reset viewport to full window
    virtual void ResetViewport() = 0;
    
    // Font rendering
    // Load a font from file
    virtual bool LoadFont(const std::string& font_path, i32 size) = 0;
    
    // Set default font size (for the loaded font)
    virtual void SetFontSize(i32 size) = 0;
    
    // Render text at position
    virtual void DrawText(const std::string& text, i32 x, i32 y, u8 r, u8 g, u8 b, u8 a = 255) = 0;
    
    // Get text dimensions (width and height)
    virtual void GetTextSize(const std::string& text, i32& width, i32& height) = 0;
};

// Platform handle types (opaque pointers)
struct WindowHandle {
    void* ptr = nullptr;
};

struct RendererHandle {
    void* ptr = nullptr;
};

} // namespace Platform
