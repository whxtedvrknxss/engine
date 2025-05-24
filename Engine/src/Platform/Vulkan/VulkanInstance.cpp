#include "VulkanInstance.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

void VulkanInstance::Create( bool enable_validation ) {
  m_EnableValidation = enable_validation;

  VkApplicationInfo app_info = {};
  app_info.pApplicationName( "Engine" );
  app_info.pEngineName( "Engine" );
  app_info.applicationVersion( VK_MAKE_API_VERSION( 0, 0, 0, 1 ) );
  app_info.engineVersion( VK_MAKE_API_VERSION( 0, 0, 0, 1 ) );
  app_info.apiVersion( VK_API_VERSION_1_2 );

  auto extensions = GetRequiredExtensions();
  vk::InstanceCreateInfo instance_info = {};
  instance_info
    .setPApplicationInfo( &app_info )
    .setPEnabledExtensionNames( extensions );

  vk::DebugUtilsMessengerCreateInfoEXT debug_info = {};
  if ( m_EnableValidation ) {
    instance_info
      .setPEnabledLayerNames( { "VK_LAYER_KHRONOS_validation" } );

    debug_info
      .setMessageSeverity(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
      )
      .setMessageType(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
      );
  }

  m_Instance = vk::createInstance( instance_info );
}

vk::Instance VulkanInstance::Get() const {
  return m_Instance;
}

std::vector<const char*> VulkanInstance::GetRequiredExtensions() const {
  uint32_t count = 0;
  const char** required_extensions = glfwGetRequiredInstanceExtensions( &count );

  std::vector extensions( required_extensions, required_extensions + count );

  if ( m_EnableValidation ) {
    extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
  }

  std::vector available = vk::enumerateInstanceExtensionProperties();

  for ( const char* name : extensions ) {
    auto it = std::ranges::find_if(
      available,
      [name] ( const vk::ExtensionProperties& ext ) {
        return std::strcmp( name, ext.extensionName.data() );
      }
    );

    if ( it == available.end() ) {
      throw std::runtime_error( std::format( "extension {0} not supported", name ) );
    }
  }

  return extensions;
}
