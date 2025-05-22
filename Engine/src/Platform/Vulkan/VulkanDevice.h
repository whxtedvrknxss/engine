// src/Engine/Renderer/Core/VulkanDevice.h

#ifndef __renderer_vulkan_device_h_included__
#define __renderer_vulkan_device_h_included__

#include <vulkan/vulkan.hpp>

struct VulkanDeviceFeatures {
  std::vector<const char*> Extensions;
  std::vector<const char*> Layers;
};

class VulkanDevice {
public:
  VulkanDevice();

private:
  vk::PhysicalDevice m_PhysicalDevice;
  vk::Device m_LogicalDevice;
  vk::Queue m_GraphicsQueue;
};

#endif 