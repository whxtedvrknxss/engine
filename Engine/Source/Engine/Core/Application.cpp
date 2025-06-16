#include "Application.h"

#include <SDL3/SDL.h>

Application::Application() {
    if ( !SDL_Init( SDL_INIT_VIDEO ) ) {
        
    }
}

Application::~Application() {
    SDL_Quit();
}
