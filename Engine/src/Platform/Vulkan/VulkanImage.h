// src/Platform/Vulkan/VulkanImage.h

#ifndef __renderer_vulkan_image_h_included__ 
#define __renderer_vulkan_image_h_included__

#include <vulkan/vulkan.h>

#include "VulkanDevice.h"

class VulkanImage {
public:
    enum Type {
        Texture = 0,
        Color = 1,
        Depth = 2,
    };

private:
    VkImage Image;
    VkImageView ImageView;
    VkDeviceMemory DeviceMemory;

    VulkanDevice* DeviceHandle;
};

#endif 
