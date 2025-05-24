#include "VulkanSurface.h"

#include <stdexcept>

#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>

#include "Engine/Core/Common.h"

void VulkanSurface::Create(const VulkanInstance& instance, GLFWwindow* window) {
  m_InstanceHandle = instance;

  VkResult res = glfwCreateWindowSurface(
    m_InstanceHandle.Get(),
    window,
    nullptr,
    &m_Surface
  );

  if ( res != VK_SUCCESS ) {
    throw std::runtime_error( "error creating Vulkan instance!" );
  }
}

void VulkanSurface::Cleanup( ) {
  vkDestroySurfaceKHR( m_InstanceHandle.Get(), m_Surface, nullptr );
}
