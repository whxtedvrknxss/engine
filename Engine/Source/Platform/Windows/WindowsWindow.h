// src/Platform/WindowsWindow.h

#ifndef __windows_window_h_included__
#define __windows_window_h_included__

#include "Engine/Core/Window.h"
#include "Engine/Core/Common.h"
#include "Engine/RHI/RHI.h"

class WindowsWindow : public WindowBase
{
public:
    WindowsWindow() = delete;
    WindowsWindow( const WindowCreateInfo& create_info );
    ~WindowsWindow() override;

    void OnUpdate() override;
    virtual void* GetNativeWindow() const { return Window; }

private:
    SDL_Window* Window;

    struct
    {
        Vec2<uint32_t> Position;
        Vec2<uint32_t> Size;
        std::string    Title;
        bool           VSync;
    } Data;

    Scope<RHIContext> Context;
};

#endif

