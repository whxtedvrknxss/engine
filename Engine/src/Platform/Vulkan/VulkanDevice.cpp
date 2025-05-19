#include "VulkanDevice.h"

#include <stdexcept>

#include "Engine/Core/Common.h"
#include "GLFW/glfw3.h"

#include "VulkanContext.h"

VulkanDevice::VulkanDevice() {
  CreateInstance();
}

void VulkanDevice::CreateInstance() {

  VkApplicationInfo app_info {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Engine";
  app_info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
  app_info.pEngineName = "Engine";
  app_info.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo instance_info {};
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_info.pApplicationInfo = &app_info;
  
  // extensions and debug info here

  const char** ptr = STATIC_CAST( const char**, malloc( 3 * sizeof( const char* ) ) );
  for (int i = 0; i < 3; ++i) {
    ptr[i] = STATIC_CAST( const char*, malloc( 20 * sizeof( char ) ) );
  }

  auto res = vkCreateInstance( &instance_info, nullptr, &m_Instance );
  if ( res != VK_SUCCESS ) {
    std::runtime_error( "failed to create vulkan instance!" );
  }
}
