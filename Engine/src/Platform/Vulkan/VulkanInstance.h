// src/Platform/Vulkan/VulkanInstance.h

#ifndef __renderer_vulkan_instance_h_included__
#define __renderer_vulkan_instance_h_included__

#include <vector>

#include <vulkan/vulkan.h>

class VulkanInstance {
public:
  void Create( bool enable_validation = true );
  VkInstance Get() const {
    return m_Instance;
  }

private:
  std::vector<const char*> GetRequiredExtensions() const;
  bool CheckExtensionSupport() const;

private:
  bool m_EnableValidation;

  VkInstance m_Instance;
  VkDebugUtilsMessengerEXT m_DebugMessenger;
};

#endif 