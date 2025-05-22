#include "GraphicsContext.h"

#include "Platform/Vulkan/VulkanContext.h"

Scope<GraphicsContext> GraphicsContext::Create(void* window) {
  switch ( 0 ) {
    case 0: {
      return CreateScope<VulkanContext>( STATIC_CAST( GLFWwindow*, window ) );
    } 
  }

  return nullptr;
}
