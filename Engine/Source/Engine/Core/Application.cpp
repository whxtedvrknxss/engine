#include "Application.h"

#include <stdexcept>
#include <chrono>

#include <SDL3/SDL.h>

std::filesystem::path Application::ExePath;

Application::Application( int argc, char* argv[] )
{
    if ( !SDL_Init( SDL_INIT_VIDEO ) )
    {
        throw std::runtime_error( "" );
    }

    WindowCreateInfo window_info = {};
    window_info.Position = { 100, 100 };
    window_info.Size = { 800, 500 };
    window_info.Title = "some title";
    Window = WindowBase::Create( window_info );

	ExePath = argv[1];
    std::printf( ExePath.string().c_str() );
}

Application::~Application()
{
    SDL_Quit();
}

int32 Application::Run()
{
    bool running = true;
    auto last_time = std::chrono::high_resolution_clock::now();
    int32 frame_count = 0;

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

        frame_count++;
        auto current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration<float>( current_time - last_time ).count();

        if ( elapsed >= 1.0f )
        {
            SDL_SetWindowTitle( ( SDL_Window* ) Window->GetNativeWindow(), std::format( "{} fps", frame_count ).c_str() );
            frame_count = 0;
            last_time = current_time;
        }
    }
    return 0;
}

