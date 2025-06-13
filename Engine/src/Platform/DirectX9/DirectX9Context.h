//

#ifndef __renderer_directx9_context_h_included__
#define __renderer_directx9_context_h_included__

#include "Engine/Renderer/GraphicsContext.h"

#include <d3d9.h>
#include <SDL3/SDL.h>

#pragma comment(lib, "d3d9.lib")

class DirectX9Context : public GraphicsContext {
public:
    DirectX9Context( SDL_Window* window_handle );

    void Init() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Cleanup() override;
    void SwapBuffers() override;

private:
    SDL_Window* WindowHandle = nullptr;
    SDL_Renderer* Renderer = nullptr;

    std::unique_ptr<IDirect3DDevice9> Device = nullptr;
};

#endif 
