// src/Engine/Renderer/Core/VulkanContext.h
  
#ifndef __renderer_vulkan_context_h_included__
#define __renderer_vulkan_context_h_included__  

#include <vulkan/vulkan.hpp>

#include "VulkanInstance.h"
#include "Engine/Renderer/GraphicsContext.h"

struct GLFWwindow;

class VulkanContext : public GraphicsContext {
public:
  VulkanContext( GLFWwindow* window_handle );

private:
  GLFWwindow* m_WindowHandle;

  VulkanInstance m_Instance;
};

#endif 
