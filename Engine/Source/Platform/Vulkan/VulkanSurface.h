// src/Platform/Vulkan/VulkanSurface.h

#ifndef __renderer_vulkan_surface_h_included__
#define __renderer_vulkan_surface_h_included__

#include <vulkan/vulkan.h>

#include "VulkanInstance.h"

struct SDL_Window;

class VulkanSurface {
public:
    void Create( VulkanInstance* instance, SDL_Window* window );
    void Cleanup();

    VkSurfaceKHR Get() const { return Surface; }

private:
    VkSurfaceKHR Surface = VK_NULL_HANDLE;

    VulkanInstance* InstanceHandle = nullptr;
};

#endif 