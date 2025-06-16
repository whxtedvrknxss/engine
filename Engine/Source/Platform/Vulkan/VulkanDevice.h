// src/Engine/Renderer/Core/VulkanDevice.h

#ifndef __renderer_vulkan_device_h_included__
#define __renderer_vulkan_device_h_included__

#include <optional>
#include <vulkan/vulkan.h>

#include "VulkanContext.h"

struct QueueFamiliesIndices {
    std::optional<uint32> Graphics;
    std::optional<uint32> Present;

    bool IsComplete() const { 
        return Graphics.has_value() && Present.has_value();
    }
};

class VulkanDevice {
public:
    VulkanDevice() = default;

    void Create( const VulkanContextCreateInfo& context_info, VulkanInstance* instance );
    void Cleanup();

    VkPhysicalDevice GetPhysical() const { return Physical; }
    VkDevice GetLogical() const { return Logical; }

    static QueueFamiliesIndices FindQueueFamilies( VkPhysicalDevice device );
private:
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    static bool CheckPhysicalDevice( VkPhysicalDevice device );

private:
    VkPhysicalDevice Physical = VK_NULL_HANDLE;
    VkDevice Logical = VK_NULL_HANDLE;

    VkQueue GraphicsQueue = VK_NULL_HANDLE;

    VulkanInstance* InstanceHandle = nullptr;
};

#endif
 