// src/Platform/Vulkan/VulkanInstance.h

#ifndef __renderer_vulkan_instance_h_included__
#define __renderer_vulkan_instance_h_included__

#include <vector>

#include <vulkan/vulkan.h>

struct VulkanInstanceFeatures {
  std::vector<const char*> Extensions;
  std::vector<const char*> Layers;
};

class VulkanInstance {
public:
  void Create( bool enable_validation = true );
  void Cleanup();

  VkInstance Get() const { return m_Instance; }

  const VulkanInstanceFeatures& GetFeatures() const { return m_Features; }

private:
  std::vector<const char*> GetRequiredExtensions() const;
  bool CheckExtensionSupport() const;

private:
  VulkanInstanceFeatures m_Features;

  bool m_EnableValidation;

  VkInstance m_Instance;
  VkDebugUtilsMessengerEXT m_DebugMessenger;
}; 

#endif 