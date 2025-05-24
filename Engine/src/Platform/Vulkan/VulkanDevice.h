// src/Engine/Renderer/Core/VulkanDevice.h

#ifndef __renderer_vulkan_device_h_included__
#define __renderer_vulkan_device_h_included__

#include <vulkan/vulkan.h>

#include "VulkanInstance.h"
#include "VulkanSurface.h"

struct VulkanDeviceFeatures {
  std::vector<const char*> Extensions;
  std::vector<const char*> Layers;
};

class VulkanDevice {
public:
  void Create( VulkanInstance instance, VulkanSurface surface );

private:
  void PickPhysicalDevice();
  void CreateLogicalDevice();

private:
  VulkanInstance& m_InstanceHandle;
  
private:
  VkPhysicalDevice m_PhysicalDevice;
  VkDevice m_LogicalDevice;
  VkQueue m_GraphicsQueue;
};

#endif 