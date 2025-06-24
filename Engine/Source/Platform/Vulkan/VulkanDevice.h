// src/Engine/Renderer/Core/VulkanDevice.h

#ifndef __renderer_vulkan_device_h_included__
#define __renderer_vulkan_device_h_included__

#include <optional>
#include <vulkan/vulkan.h>

#include "VulkanContext.h"

class VulkanSurface;

struct QueueFamilyIndices {
    std::optional<uint32> Graphics;
    std::optional<uint32> Present;
    std::optional<uint32> Compute;
    std::optional<uint32> Transfer;

    bool IsComplete() const { 
        return Graphics.has_value() && Present.has_value();
    }

    bool HasDedicatedCompute() const {
        return Compute.has_value() && Compute != Graphics;
    }

    bool HasDedicatedTransfer() const {
        return Transfer.has_value() && Transfer != Graphics;
    }
};

class VulkanDevice {
public:
    VulkanDevice() = default;

    void Create( const VulkanContextCreateInfo& context_info, VulkanInstance* instance_handle, 
        VulkanSurface* surface_handle );
    void Cleanup();

    VkPhysicalDevice GetPhysical() const { return Physical; }
    VkDevice GetLogical() const { return Logical; }

    void FindQueueFamilies( VkSurfaceKHR surface );

private:
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    static bool CheckPhysicalDevice( VkPhysicalDevice device );

private:
    QueueFamilyIndices QueueFamilies;

    VkPhysicalDevice Physical = VK_NULL_HANDLE;
    VkDevice         Logical = VK_NULL_HANDLE;

    VkQueue          GraphicsQueue = VK_NULL_HANDLE;
    VkQueue          PresentQueue = VK_NULL_HANDLE;
    VkQueue          ComputeQueue = VK_NULL_HANDLE;
    VkQueue          TransferQueue = VK_NULL_HANDLE;

    VulkanInstance*  InstanceHandle = nullptr;
};

#endif
 