#include "VulkanSurface.h"

#include <stdexcept>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

void VulkanSurface::Create( VulkanInstance* instance, SDL_Window* window ) {
  InstanceHandle = instance;

  if ( !SDL_Vulkan_CreateSurface( window, InstanceHandle->Get(), nullptr, &Surface ) ) {
    throw std::runtime_error( "error creating Vulkan instance!" );
  }
}

void VulkanSurface::Cleanup( ) {
  SDL_Vulkan_DestroySurface( InstanceHandle->Get(), Surface, nullptr );
}
