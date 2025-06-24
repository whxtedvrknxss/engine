#include "VulkanContext.h"

#include <set>
#include <stdexcept>

#include <SDL3/SDL_vulkan.h>>

#include "Engine/Core/Common.h"

VulkanContext::VulkanContext( VulkanContextCreateInfo&& context_info, SDL_Window* window_handle ) {
    ContextInfo = std::move( context_info );
    WindowHandle = window_handle;

    Instance = VK_NULL_HANDLE;
    Surface = VK_NULL_HANDLE;
    PhysicalDevice = VK_NULL_HANDLE;
    Device = VK_NULL_HANDLE;
    GraphicsQueue = VK_NULL_HANDLE;
    PresentQueue = VK_NULL_HANDLE;
}

void VulkanContext::Init() {
    Instance = CreateInstance();
    Surface = CreateSurface();

    PhysicalDevice = PickPhysicalDevice();

    QueueFamilyIndices indices = FindQueueFamilies( PhysicalDevice );

    Device = CreateDevice( indices );
    GraphicsQueue = GetQueue( indices.Graphics.value(), 0 );
    PresentQueue = GetQueue( indices.Present.value(), 0 );
}

void VulkanContext::Cleanup() {
    vkDestroyDevice( Device, nullptr );
    vkDestroySurfaceKHR( Instance, Surface, nullptr );
    vkDestroyInstance( Instance, nullptr );
}

void VulkanContext::CheckVulkanResult( VkResult err ) {
    if ( err == VK_SUCCESS ) {
        return;
    }

    fprintf( stderr, "Vulkan error: %d", err );
    abort();
}

bool VulkanContext::IsExtensionAvailable( const std::vector<VkExtensionProperties>& props, const char* extension ) {
    for ( const auto& prop : props ) {
        if ( strcmp( prop.extensionName, extension ) == 0 ) {
            return true;
        }
    }
    return false;
}

bool VulkanContext::IsLayerAvailable( const std::vector<VkLayerProperties>& props, const char* layer ) {
    for ( const auto& prop : props ) {
        if ( strcmp( prop.layerName, layer ) == 0 ) {
            return true;
        }
    }
    return false;
}

VkInstance VulkanContext::CreateInstance() {
    VkResult err;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = ContextInfo.ApplicationName;
    app_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 0 );
    app_info.pEngineName = ContextInfo.EngineName;
    app_info.engineVersion = VK_MAKE_VERSION( 0, 0, 0 );
    app_info.apiVersion = VK_MAKE_API_VERSION( 0, ContextInfo.VulkanApiMajorVersion,
        ContextInfo.VulkanApiMinorVersion, 0 );

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;

    uint32 count_ext = 0;
    err = vkEnumerateInstanceExtensionProperties( nullptr, &count_ext, nullptr );
    CheckVulkanResult( err );

    std::vector<VkExtensionProperties> available_ext( count_ext );
    err = vkEnumerateInstanceExtensionProperties( nullptr, &count_ext, available_ext.data() );
    CheckVulkanResult( err );

    for ( const auto& ext : ContextInfo.Extensions ) {
        if ( !IsExtensionAvailable( available_ext, ext ) ) {
            throw std::runtime_error( "extension is not available" );
        }
    }

    instance_info.enabledExtensionCount = STATIC_CAST( uint32, ContextInfo.Extensions.size() );
    instance_info.ppEnabledExtensionNames = ContextInfo.Extensions.data();

    uint32 count_layers = 0;
    err = vkEnumerateInstanceLayerProperties( &count_layers, nullptr );
    CheckVulkanResult( err );

    std::vector<VkLayerProperties> available_layers( count_layers );
    err = vkEnumerateInstanceLayerProperties( &count_layers, available_layers.data() );
    CheckVulkanResult( err );

    for ( const auto& layer : ContextInfo.Layers ) {
        if ( !IsLayerAvailable( available_layers, layer ) ) {
            throw std::runtime_error( "layer is not available" );
        }
    }

    instance_info.enabledLayerCount = STATIC_CAST( uint32, ContextInfo.Layers.size() );
    instance_info.ppEnabledLayerNames = ContextInfo.Layers.data();

    VkInstance instance;
    err = vkCreateInstance( &instance_info, nullptr, &instance );
    CheckVulkanResult( err );

    return instance;
}

VkSurfaceKHR VulkanContext::CreateSurface() {
    VkSurfaceKHR surface;
    if ( !SDL_Vulkan_CreateSurface( WindowHandle, Instance, nullptr, &surface ) ) {
        fprintf( stderr, "Error creating window surface: %s", SDL_GetError() );
    }
    return surface;
}

VkPhysicalDevice VulkanContext::PickPhysicalDevice() {
    VkResult err;

    uint32 count = 0;
    err = vkEnumeratePhysicalDevices( Instance, &count, nullptr );
    CheckVulkanResult( err );

    if ( count == 0 ) {
        throw std::runtime_error( "failed to find GPUs with Vulkan support" );
    }

    std::vector<VkPhysicalDevice> devices( count );
    err = vkEnumeratePhysicalDevices( Instance, &count, devices.data() );
    CheckVulkanResult( err );

    for ( const auto& device : devices ) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties( device, &props );

        //VkPhysicalDeviceFeatures features;
        //vkGetPhysicalDeviceFeatures( device, &features );

        //QueueFamilyIndices indices = FindQueueFamilies( device );

        //uint32 count_extensions = 0;
        //err = vkEnumerateDeviceExtensionProperties( device, nullptr, &count, nullptr );
        //CheckVulkanResult( err );

        //std::vector<VkExtensionProperties> available_extensions( count_extensions );
        //err = vkEnumerateDeviceExtensionProperties( device, nullptr, &count, available_extensions.data() );
        //CheckVulkanResult( err );

        //bool swapchain_support = false;
        //for ( const auto& ext : available_extensions ) {
        //    if ( ext.extensionName == VK_KHR_SWAPCHAIN_EXTENSION_NAME ) {
        //        swapchain_support = true;
        //        break;
        //    }
        //}

        if ( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU /* && swapchain_support */) {
            return device;
        }
    }

    return devices[0];
}

QueueFamilyIndices VulkanContext::FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface ) {
    uint32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &count, nullptr );
    std::vector<VkQueueFamilyProperties> props( count );

    QueueFamilyIndices indices;
    int32 i = 0;
    for ( const auto& queue_family : props ) {
        if ( queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
            indices.Graphics = i;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &present_support );

        if ( present_support ) {
            indices.Present = i;
        }

        if ( indices.IsComplete() ) {
            break;
        }

        ++i;
    }
    return indices;
}

VkDevice VulkanContext::CreateDevice( QueueFamilyIndices indices ) {
    std::vector<VkDeviceQueueCreateInfo> queue_infos = {};
    std::set<uint32> unique_families = {
        indices.Graphics.value(),
        indices.Present.value()
    };

    float priority = 1.0f;
    for ( uint32 family : unique_families ) {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_infos.push_back( queue_info );
    }

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = STATIC_CAST( uint32, queue_infos.size() );
    device_info.pQueueCreateInfos = queue_infos.data();

    VkPhysicalDeviceFeatures physical_device_features = {};
    device_info.pEnabledFeatures = &physical_device_features;
    device_info.enabledLayerCount = STATIC_CAST( uint32, ContextInfo.Layers.size() );
    device_info.ppEnabledLayerNames = ContextInfo.Layers.data();

    // the extension requires check for availability but i don't really care since i got 4060
    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    device_info.enabledExtensionCount = STATIC_CAST( uint32, device_extensions.size() );
    device_info.ppEnabledExtensionNames = device_extensions.data();

    VkDevice device;
    VkResult err = vkCreateDevice( PhysicalDevice, &device_info, nullptr, &device );
    CheckVulkanResult( err );

    return device;
}

VkQueue VulkanContext::GetQueue( uint32 family_index, uint32 index ) {
    VkQueue queue;
    vkGetDeviceQueue( Device, family_index, index, &queue );
    return queue;
}

VkSwapchainKHR VulkanContext::CreateSwapchain() {
    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = Surface;

    SDL_GetWindowSize();
}

