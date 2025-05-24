// src/Platform/Vulkan/VulkanSurface.h

#ifndef __renderer_vulkan_surface_h_included__
#define __renderer_vulkan_surface_h_included__

#include <vulkan/vulkan.h>

#include "VulkanInstance.h"

struct GLFWwindow;

class VulkanSurface {
public:
  void Create( const VulkanInstance& instance, GLFWwindow* window );
  void Cleanup();

private:
  VkSurfaceKHR m_Surface;

  VulkanInstance& m_InstanceHandle;
};

#endif 