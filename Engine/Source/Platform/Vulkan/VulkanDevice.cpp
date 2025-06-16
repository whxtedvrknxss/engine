#include "VulkanDevice.h"

#include "VulkanCall.h"

#include "Engine/Core/Log.h"
#include "Engine/Core/Common.h"

void VulkanDevice::Create( const VulkanContextCreateInfo& context_info, VulkanInstance* instance ) {
    PickPhysicalDevice();

    const VulkanContextCreateInfo*  ptr = &context_info;
}

void VulkanDevice::Cleanup() {
    vkDestroyDevice(Logical, nullptr);
}

void VulkanDevice::PickPhysicalDevice() {
    uint32 device_count = 0;
    VK_CALL( vkEnumeratePhysicalDevices( InstanceHandle->Get(), &device_count, nullptr ) );

    if ( device_count == 0 ) {
        LOG_ERROR( "Failed to find GPUs with Vulkan support" );
        throw std::runtime_error( "no gpu available" );
    }

    VkPhysicalDevice* devices = nullptr;
    VK_CALL( vkEnumeratePhysicalDevices( InstanceHandle->Get(), &device_count, devices ) );

    for ( int32 i = 0; i < device_count; ++i ) {
        if ( CheckPhysicalDevice( devices[i] ) ) {
            Physical = devices[i];
        }
    }

    if ( Physical == VK_NULL_HANDLE ) {
        LOG_ERROR( "Failed to find GPU with necessary features" );
        throw std::runtime_error( "no gpu features available" );
    }

    // TODO: rate devices by suitability
}

void VulkanDevice::CreateLogicalDevice() {
    QueueFamiliesIndices indices = FindQueueFamilies( Physical );

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

QueueFamiliesIndices VulkanDevice::FindQueueFamilies( VkPhysicalDevice device ) {
    QueueFamiliesIndices indices;

    uint32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &count, nullptr );

    VkQueueFamilyProperties* properties = nullptr;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &count, properties);

    for ( uint32 i = 0; i < count; ++i ) {
        if ( properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
            indices.Graphics = i;
        }

        if ( indices.IsComplete() ) {
            break;
        }
    }
    
    return indices;
}
