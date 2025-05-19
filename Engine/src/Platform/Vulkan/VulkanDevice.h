// src/Platform/Vulkan/Device.h

#ifndef __vulkan_device_h_included__
#define __vulkan_device_h_included__

#include <vulkan/vulkan.h>

class VulkanDevice {
public:
  VulkanDevice();

private:
  void CreateInstance();



private:
  VkInstance m_Instance;
  VkPhysicalDevice m_PhysicalDevice;
  VkDevice m_Device;
};

#endif 
