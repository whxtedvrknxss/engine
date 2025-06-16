// src/Platform/Vulkan/VulkanInstance.h

#ifndef __renderer_vulkan_instance_h_included__
#define __renderer_vulkan_instance_h_included__

#include <vector>

#include <vulkan/vulkan.h>

struct VulkanContextCreateInfo;

class VulkanInstance {
public:
    VulkanInstance() = default;

    void Create( const VulkanContextCreateInfo& context_info );
    void Cleanup();

    VkInstance Get() const { return Instance;  }

private:
    static bool CheckRequiredExtensionSupport( const std::vector<const char*>& required );
    static bool CheckRequiredLayerSupport( const std::vector<const char*>& required );

private:
    VkInstance Instance = VK_NULL_HANDLE;
};

#endif 