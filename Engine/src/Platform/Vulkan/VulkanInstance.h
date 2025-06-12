// src/Platform/Vulkan/VulkanInstance.h

#ifndef __renderer_vulkan_instance_h_included__
#define __renderer_vulkan_instance_h_included__

#include <vector>

#include <vulkan/vulkan.h>

class VulkanInstance {
public:
  struct Features {
    std::vector<const char*> Extensions;
    std::vector<const char*> Layers;
  };

  void Create( bool enable_validation = true );
  void Cleanup();

  VkInstance Get() const { return Instance; }

  const Features& GetFeatures() const { return Features; }

private:
  std::vector<const char*> GetRequiredExtensions() const;
  bool CheckExtensionSupport() const;

private:
  Features Features;

  bool EnableValidation;

  VkInstance Instance;
  VkDebugUtilsMessengerEXT DebugMessenger;
}; 

#endif 