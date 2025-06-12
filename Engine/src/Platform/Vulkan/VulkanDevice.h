// src/Engine/Renderer/Core/VulkanDevice.h

#ifndef __renderer_vulkan_device_h_included__
#define __renderer_vulkan_device_h_included__

#include <optional>

#include <vulkan/vulkan.h>

#include "VulkanInstance.h"
#include "VulkanSurface.h"

struct QueueFamilyIndices {
  std::optional<uint32_t> GraphicsFamily;
  std::optional<uint32_t> PresentFamily;

  bool IsComplete() const {
    return GraphicsFamily.has_value() && PresentFamily.has_value();
  }
};

class VulkanDevice {
public:
  struct Features {
    std::vector<const char*> Extensions;
  };

  void Create( VulkanInstance* instance, VulkanSurface* surface );

  VkPhysicalDevice Physical() const { return PhysicalDevice; }
  VkDevice Logical() const { return LogicalDevice; }

  static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice physical_device,
    VkSurfaceKHR surface );

  const Features& GetFeatures() { return Features; }

private:
  static bool IsDeviceSuitable( VulkanDevice* device, VulkanSurface* surface );
  static bool CheckDeviceExtensionSupport( VulkanDevice* device );

  void PickPhysical();
  void CreateLogical();

private:
  VulkanInstance* InstanceHandle = nullptr;
  VulkanSurface* SurfaceHandle = nullptr;

  Features Features;
  
  VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
  VkDevice LogicalDevice          = VK_NULL_HANDLE;
  VkQueue GraphicsQueue           = VK_NULL_HANDLE;
  VkQueue PresentQueue            = VK_NULL_HANDLE;
}; 

#endif
 