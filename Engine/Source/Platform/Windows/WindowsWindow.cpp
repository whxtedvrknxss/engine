#include "WindowsWindow.h"
#include "Engine/Core/Log.h"

WindowsWindow::WindowsWindow( const WindowCreateInfo& create_info )
{
    Data.Position = create_info.Position;
    Data.Size = create_info.Size;
    Data.Title = create_info.Title;
    Data.VSync = false;

    int32 x = Data.Size.X;
    int32 y = Data.Size.Y;

    Window = SDL_CreateWindow( 
        Data.Title.c_str(), 
        x, y,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE );

    Context = RHIContext::Create( Window, RHIContext::Backend::Vulkan );
    try
    {
        Context->Init();
    }
    catch ( const std::exception& ex )
    {
		LOG_ERROR( ex.what() );
    }
}

WindowsWindow::~WindowsWindow()
{
    SDL_DestroyWindow( Window );
}

void WindowsWindow::OnUpdate()
{
    try
    {
        Context->BeginFrame();
        Context->DrawFrame();
        Context->EndFrame();
    }
    catch ( const std::exception& ex )
    {
		LOG_ERROR( ex.what() );
    }
}

