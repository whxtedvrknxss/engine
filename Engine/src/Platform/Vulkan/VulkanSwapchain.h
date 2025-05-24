// src/Platform/Vulkan/VulkanSwapchain.h

#ifndef __renderer_vulkan_swapchain_h_included__
#define __renderer_vulkan_swapchain_h_included__

#include <vulkan/vulkan.h>

#include "VulkanDevice.h"

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR Capabilities;
  std::vector<VkSurfaceFormatKHR> Formats;
  std::vector<VkPresentModeKHR> PresentModes;
};

class VulkanSwapchain {
public:
  void Create( VulkanDevice* device, VulkanSurface* surface );
  void Cleanup();

  void Recreate();

private:
  SwapChainSupportDetails QuerySwapChainSupport() const;
  VkSurfaceFormatKHR ChooseSwapSurfaceFormat();
  VkPresentModeKHR ChooseSwapPresentMode();
  VkExtent2D ChooseSwapExtent();

private:
  VulkanDevice* m_DeviceHandle = nullptr;
  VulkanSurface* m_SurfaceHandle = nullptr;

private:
  VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
  VkExtent2D m_SwapchainExtent;
  std::vector<VkImage> m_Images;
  std::vector<VkImageView> m_ImageViews;
  std::vector<VkFramebuffer> m_Framebuffers;
};

#endif

