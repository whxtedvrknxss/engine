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
  void Create( VulkanDevice* device, VulkanSurface* surface, GLFWwindow* window );
  void Cleanup();


  void Recreate();

  static SwapChainSupportDetails QuerySwapChainSupport(const VulkanDevice* device, 
    const VulkanSurface* surface);

private:
  void CreateImageViews();

  static VkImageView CreateImageView( VulkanDevice* device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags,
    uint32_t mip_levels );

  VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& available ) const;
  VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& available ) const;
  VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) const;

private:
  VulkanDevice* m_DeviceHandle = nullptr;
  VulkanSurface* m_SurfaceHandle = nullptr;
  GLFWwindow* m_WindowHandle = nullptr;

private:
  VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
  VkExtent2D m_Extent;
  
  std::vector<VkImage> m_Images;
  std::vector<VkImageView> m_ImageViews;
  VkFormat m_ImageFormat;

  std::vector<VkFramebuffer> m_Framebuffers;

};

#endif

