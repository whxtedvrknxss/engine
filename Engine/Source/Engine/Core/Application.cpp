#include "Application.h"

#include <stdexcept>

#include <SDL3/SDL.h>

Application::Application()
{
    if ( !SDL_Init( SDL_INIT_VIDEO ) )
    {
        throw std::runtime_error( "" );
    }

    WindowCreateInfo window_info = {};
    window_info.Position = { 100,100 };
    window_info.Size = { 800,500 };
    window_info.Title = "some title";
    Window = WindowBase::Create( window_info );
}

Application::~Application()
{
    SDL_Quit();
}

int32 Application::Run()
{
    bool running = true;
    while ( running )
    {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) )
        {
            if ( event.type == SDL_EVENT_QUIT )
            {
                running = false;
            }
        }

        Window->OnUpdate();
    }
    return 0;
}

