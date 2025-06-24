#include "VulkanDevice.h"

#include "VulkanCall.h"
#include "VulkanSurface.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Common.h"

void VulkanDevice::Create( const VulkanContextCreateInfo& context_info, VulkanInstance* instance, 
    VulkanSurface* surface_handle ) {
    PickPhysicalDevice();
    CreateLogicalDevice();
}

void VulkanDevice::Cleanup() {
    vkDestroyDevice(Logical, nullptr);
}

void VulkanDevice::FindQueueFamilies( VkSurfaceKHR surface ) {
    uint32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( Physical, &count, nullptr );

    VkQueueFamilyProperties* properties = nullptr;
    vkGetPhysicalDeviceQueueFamilyProperties( Physical, &count, properties);

    for ( uint32 i = 0; i < count; ++i ) {
        if ( properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
            QueueFamilies.Graphics = i;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR( Physical, i, surface, &present_support );

        if ( present_support ) {
            QueueFamilies.Present = i;
        }

        if ( QueueFamilies.IsComplete() ) {
            break;
        }
    }
}

void VulkanDevice::PickPhysicalDevice() {
    uint32 gpu_count = 0;
    vkEnumeratePhysicalDevices( InstanceHandle->Get(), &gpu_count, nullptr );

    std::vector<VkPhysicalDevice> gpus( gpu_count );
    vkEnumeratePhysicalDevices( InstanceHandle->Get(), &gpu_count, gpus.data() );

    for ( const auto gpu : gpus ) {
        VkPhysicalDeviceProperties gpu_props = {};
        vkGetPhysicalDeviceProperties( gpu, &gpu_props );

        if ( gpu_props.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) {
            //return gpu;
        }
    }

    if ( gpu_count > 0 ) {
        //return gpus[0];
    }

    //return VK_NULL_HANDLE;
}

void VulkanDevice::CreateLogicalDevice() {
    QueueFamilyIndices indices;//FindQueueFamilies( Physical );

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = indices.Graphics.value();
    queue_info.queueCount = 1;

    float priority = 1.0f;
    queue_info.pQueuePriorities = &priority;
 
    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.queueCreateInfoCount = 1;

    VkPhysicalDeviceFeatures device_features = {};
    device_info.pEnabledFeatures = &device_features;

    VK_CALL( vkCreateDevice( Physical, &device_info, nullptr, &Logical ) );

    vkGetDeviceQueue( Logical, indices.Graphics.value(), 0, &GraphicsQueue );
}

bool VulkanDevice::CheckPhysicalDevice( VkPhysicalDevice device ) {
    

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties( device, &properties );

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures( device, &features );

    return ( properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        features.geometryShader );

    // TODO: replace by RateDevice function 
}

