#include "VulkanInstance.h"

#include <format>
#include <stdexcept>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "Engine/Core/Common.h"

void VulkanInstance::Create( bool enable_validation ) {
  EnableValidation = enable_validation;

  Features = {
    .Extensions = GetRequiredExtensions(),
    .Layers = { "VK_LAYER_KHRONOS_validtaion" }
  };

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Engine";
  app_info.pEngineName = "Engine";
  app_info.applicationVersion = VK_MAKE_API_VERSION( 0, 0, 0, 1 );
  app_info.engineVersion = VK_MAKE_API_VERSION( 0, 0, 0, 1 );
  app_info.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo instance_info = {};
  instance_info.pApplicationInfo = &app_info;
  instance_info.enabledExtensionCount = STATIC_CAST( uint32_t, Features.Extensions.size() );
  instance_info.ppEnabledExtensionNames = Features.Extensions.data();
  instance_info.enabledLayerCount = STATIC_CAST( uint32_t, Features.Layers.size() );
  instance_info.ppEnabledLayerNames = Features.Layers.data();

  if (VkResult res = vkCreateInstance( &instance_info, nullptr, &Instance );
      res != VK_SUCCESS ) {
    throw std::runtime_error(
      "failed to create Vulkan instance. error code" 
    );
  }
}

void VulkanInstance::Cleanup() {
  vkDestroyInstance( Instance, nullptr );
}

std::vector<const char*> VulkanInstance::GetRequiredExtensions() const {
  uint32_t count = 0;
  auto required_extensions = SDL_Vulkan_GetInstanceExtensions( &count );

  std::vector extensions( required_extensions, required_extensions + count );

  if ( EnableValidation ) {
    extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
  }

  vkEnumerateInstanceExtensionProperties( nullptr, &count, nullptr );
  std::vector<VkExtensionProperties> available_extensions( count );
  vkEnumerateInstanceExtensionProperties( nullptr, &count, available_extensions.data() );

  for ( const char* name : extensions ) {
    auto it = std::ranges::find_if(
      available_extensions.begin(),
      available_extensions.end(),
      [name] ( const VkExtensionProperties& ext ) {
        return std::strcmp( name, ext.extensionName );
      }
    );

    if ( it == available_extensions.end() ) {
      throw std::runtime_error( std::format( "extension {} not supported", name ) );
    }
  }

  return extensions;
}
