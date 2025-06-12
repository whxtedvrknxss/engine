#include "VulkanContext.h"

#include <stdexcept>

#include "Engine/Core/Common.h"

#include <SDL3/SDL_vulkan.h>

static std::vector<const char*> GetRequiredExtensions() {
    uint32_t count = 0;
    auto glfw_extensions = SDL_Vulkan_GetInstanceExtensions( &count );

    std::vector extensions( glfw_extensions, glfw_extensions + count );
    extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

    return extensions;
}

static bool CheckRequestedExtensions( const std::vector<const char*>& extensions ) {
    uint32_t count = 0;
    if ( VkResult res = vkEnumerateInstanceExtensionProperties( nullptr, &count, nullptr ); res != VK_SUCCESS ) {
        std::fprintf( stderr, "Failed to enumerate Instance Extensions count. Error code: %d", res );
    }

    VkExtensionProperties* available = nullptr;
    if (VkResult res = vkEnumerateInstanceExtensionProperties( nullptr, &count, available ); res != VK_SUCCESS) {
        std::fprintf( stderr, "Failed to enumerate Instance Extensions properties. Error code: %d", res );
    }

    auto* begin = available;
    auto* end   = available + count;

    for ( const char* name : extensions ) {
        auto it = std::find_if(
            begin,
            end,
            [name] ( const VkExtensionProperties& extension ) {
                return std::strcmp( name, extension.extensionName );
            }
        );

        if ( it == end ) {
            return false;
        }
    }

    return true;
}

static bool CheckRequestedLayers( const std::vector<const char*>& layers ) {
    uint32_t count = 0;
    if ( VkResult res = vkEnumerateInstanceLayerProperties( &count, nullptr ); res != VK_SUCCESS ) {
        std::fprintf( stderr, "Failed to enumerate Instance Layer count. Error code: %d", res );
    }

    VkLayerProperties* available = nullptr;
    if ( VkResult res = vkEnumerateInstanceLayerProperties( &count, nullptr ); res != VK_SUCCESS ) {
        std::fprintf( stderr, "Failed to enumerate Instance Layer properties. Error code: %d", res );
    }

    auto* begin = available;
    auto* end   = available + count;

    for ( const char* name : layers ) {
        auto it = std::find_if(
            begin,
            end,
            [name] ( const VkLayerProperties& layer ) {
                return std::strcmp( name, layer.layerName );
            }
        );
    }

    return true;
}

VulkanContext::VulkanContext( GLFWwindow* window_handle ) {
    WindowHandle = window_handle;

    VkApplicationInfo app_info = {};
    app_info.pApplicationName = "Engine";
    app_info.pEngineName = "Engine";
    app_info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    app_info.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info = {};
    instance_info.pApplicationInfo = &app_info;

    auto extensions = GetRequiredExtensions();
    instance_info.enabledExtensionCount = STATIC_CAST( uint32_t, extensions.size() );
    instance_info.ppEnabledExtensionNames = extensions.data();
}
