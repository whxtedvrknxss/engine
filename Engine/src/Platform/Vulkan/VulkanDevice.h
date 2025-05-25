// src/Engine/Renderer/Core/VulkanDevice.h

#ifndef __renderer_vulkan_device_h_included__
#define __renderer_vulkan_device_h_included__

#include <optional>

#include <vulkan/vulkan.h>

#include "VulkanInstance.h"
#include "VulkanSurface.h"

struct VulkanDeviceFeatures {
  std::vector<const char*> Extensions;
};

struct QueueFamilyIndices {
  std::optional<uint32_t> GraphicsFamily;
  std::optional<uint32_t> PresentFamily;

  bool IsComplete() const {
    return GraphicsFamily.has_value() && PresentFamily.has_value();
  }
};

class VulkanDevice {
public:
  void Create( VulkanInstance* instance, const VulkanSurface& surface );

  VkPhysicalDevice Physical() const { return m_PhysicalDevice; }
  VkDevice Logical() const { return m_LogicalDevice; }

  static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice physical_device,
    VkSurfaceKHR surface );

private:
  static bool IsDeviceSuitable( VulkanDevice* device, VulkanSurface* surface );

  void PickPhysicalDevice();
  void CreateLogicalDevice();

private:
  VulkanInstance* m_InstanceHandle = nullptr;
  VulkanSurface* m_SurfaceHandle = nullptr;

  VulkanDeviceFeatures m_Features;
  
private:
  VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
  VkDevice m_LogicalDevice          = VK_NULL_HANDLE;
  VkQueue m_GraphicsQueue           = VK_NULL_HANDLE;
  VkQueue m_PresentQueue            = VK_NULL_HANDLE;
}; 

#endif 