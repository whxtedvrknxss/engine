#include "WindowsWindow.h"

WindowsWindow::WindowsWindow( const WindowCreateInfo& create_info ) {
    Data.Position = create_info.Position;
    Data.Size = create_info.Size;
    Data.Title = create_info.Title;

    Window = SDL_CreateWindow( Data.Title.c_str(), STATIC_CAST( int32_t, Data.Size.X ),
        STATIC_CAST( int32_t, Data.Size.Y ), SDL_WINDOW_VULKAN );
}

WindowsWindow::~WindowsWindow() {
    SDL_DestroyWindow( Window );
}

void WindowsWindow::OnUpdate() {
  //glfwSwapBuffers( Window );
}

