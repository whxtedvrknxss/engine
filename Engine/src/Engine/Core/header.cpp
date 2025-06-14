#include "header.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>

void show_window() {
    if ( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMEPAD ) ) {
        SDL_Log( "error initializing SDL! error message: %s\n", SDL_GetError() );
        __debugbreak();
    }

    SDL_Window* window = SDL_CreateWindow( "title", 700, 600, SDL_WINDOW_VULKAN );

    if ( !window ) {
        SDL_Log( "window has not been created! error message: %s\n", SDL_GetError() );
        __debugbreak();
    }

    SDL_Surface* surface = SDL_GetWindowSurface( window );

    bool running = true;
    while ( running ) {
        SDL_Event e;
        while ( SDL_PollEvent( &e ) == true ) {
            if ( e.type == SDL_EVENT_QUIT ) {
                running = false;
            }
        }
        
        SDL_FillSurfaceRect( surface, nullptr, SDL_MapSurfaceRGB( surface, 0x19, 0x19, 0x19 ) );

        SDL_UpdateWindowSurface( window );
    }

    SDL_DestroySurface( surface );
    SDL_DestroyWindow( window );
    SDL_Quit();
}

