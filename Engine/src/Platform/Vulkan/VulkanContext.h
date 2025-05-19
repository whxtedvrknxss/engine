// src/Platform/Vulkan/VulkanContext.h

#ifndef __vulkan_vulkan_context_h__
#define __vulkan_vulkan_context_h__

#include <cstdint>
#include <vector>

#include "VulkanDevice.h"

struct VulkanContextCreateInfo {
  int32_t VulkanApiMajorVersion;
  int32_t VulkanApiMinorVersion;
  std::vector<const char*> Layers;
  std::vector<const char*> Extensions;
};

class VulkanContext {
public:
  VulkanContext();

private:
  bool CheckRequestedValidationLayers(const VulkanContextCreateInfo& options) const;

private:
  VulkanDevice m_Device;
};

#endif 