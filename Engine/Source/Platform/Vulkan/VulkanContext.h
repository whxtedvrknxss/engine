// src/Engine/Renderer/Core/VulkanContext.h

#ifndef __renderer_vulkan_context_h_included__
#define __renderer_vulkan_context_h_included__  

#include "VulkanInstance.h"
#include "Engine/Renderer/GraphicsContext.h"

struct SDL_Window;

struct VulkanContextCreateInfo {
    int32 VulkanApiMajorVersion;
    int32 VulkanApiMinorVersion;
    std::vector<const char*> Extensions;
    std::vector<const char*> Layers;
    const char* ApplicationName;
    const char* EngineName;
};

class VulkanContext : public GraphicsContext {
public:
    VulkanContext( const VulkanContextCreateInfo& context_info, SDL_Window* window );
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

