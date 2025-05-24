#include "VulkanContext.h"

#include "Engine/Core/Common.h"
#include "GLFW/glfw3.h"

static std::vector<const char*> GetRequiredExtensions() {
  uint32_t count = 0;
  const char** glfw_extensions = glfwGetRequiredInstanceExtensions( &count );

  std::vector extensions( glfw_extensions, glfw_extensions + count );
  extensions.push_back( vk::EXTDebugUtilsExtensionName );

  return extensions;
}

static bool CheckRequestedExtensions( const std::vector<const char*>& extensions ) {
  auto available = vk::enumerateInstanceExtensionProperties();
  for ( const char* name : extensions ) {
    auto it = std::find_if(
      available.begin(),
      available.end(),
      [name] ( const vk::ExtensionProperties& extension ) {
        return std::strcmp( name, extension.extensionName.data() );
      } 
    );

    if ( it == available.end() ) {
      return false;
    }
  }

  return true;
}

static bool CheckRequestedLayers( const std::vector<const char*>& layers ) {
  uint32_t count = 0;
  vkEnumerateInstanceLayerProperties( &count, nullptr );

  std::vector<VkLayerProperties> available( count );
  vkEnumerateInstanceLayerProperties( &count, available.data() );

  for ( const char* name : layers) {
    auto it = std::find_if(
      available.begin(),
      available.end(),
      [name] ( const vk::LayerProperties& layer ) {
        return std::strcmp( name, layer.layerName.data() );
      }
    );
  }

  return true;
} 

VulkanContext::VulkanContext(GLFWwindow* window_handle) {
  m_WindowHandle = window_handle;

  vk::ApplicationInfo app_info = {};
  app_info
    .setPApplicationName( "Engine" )
    .setPEngineName( "Engine" )
    .setApplicationVersion( VK_MAKE_VERSION( 1, 0, 0 ) )
    .setEngineVersion( VK_MAKE_VERSION( 1, 0, 0 ) )
    .setApiVersion( vk::ApiVersion10 );

  vk::InstanceCreateInfo instance_info = {};
  instance_info
    .setPApplicationInfo( &app_info )
    .setEnabledLayerCount( STATIC_CAST( uint32_t, LAYERS.size() ) )
    .setPpEnabledLayerNames( LAYERS.data() );

  auto extensions = GetRequiredExtensions();
  instance_info
    .setEnabledExtensionCount( STATIC_CAST( uint32_t, extensions.size() ) )
    .setPpEnabledExtensionNames( extensions.data() );

  m_Instance = vk::createInstance( instance_info );
}
