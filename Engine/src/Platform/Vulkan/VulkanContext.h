// src/Engine/Renderer/Core/VulkanContext.h

#ifndef __renderer_vulkan_context_h_included__
#define __renderer_vulkan_context_h_included__  

#include "VulkanInstance.h"
#include "Engine/Renderer/GraphicsContext.h"

struct SDL_Window;

class VulkanContext : public GraphicsContext {
public:
    VulkanContext( SDL_Window* window_handle );
    void Init() override {}
    void BeginFrame() override {}
    void EndFrame() override {}
    void Cleanup() override {}
    void SwapBuffers() override {}

private:
    SDL_Window* WindowHandle;

    VulkanInstance Instance;
};

#endif 

