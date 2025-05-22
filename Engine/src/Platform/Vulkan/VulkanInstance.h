// src/Platform/Vulkan/VulkanInstance.h

#ifndef __renderer_vulkan_instance_h_included__
#define __renderer_vulkan_instance_h_included__

#include <vulkan/vulkan.hpp>

class VulkanInstance {
public:
  void Create( bool enable_validation = true );
  vk::Instance Get() const;

private:
  std::vector<const char*> GetRequiredExtensions() const;
  bool CheckExtensionSupport() const;

private:
  bool m_EnableValidation;

  vk::Instance m_Instance;
  vk::DebugUtilsMessengerEXT m_DebugMessenger;
};

#endif 