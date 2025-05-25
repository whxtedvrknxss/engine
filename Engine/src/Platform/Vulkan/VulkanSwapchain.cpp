#include "VulkanSwapchain.h"

#include <algorithm>
#include <format>
#include <stack>
#include <stdexcept>

#include <GLFW/glfw3.h>

#include "Engine/Core/Common.h"

void VulkanSwapchain::Create( VulkanDevice* device, VulkanSurface* surface,
  GLFWwindow* window ) {
  m_DeviceHandle = device;
  m_SurfaceHandle = surface;

  SwapChainSupportDetails details = QuerySwapChainSupport();

  VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat( details.Formats );
  VkPresentModeKHR present_mode = ChooseSwapPresentMode( details.PresentModes );
  VkExtent2D extent = ChooseSwapExtent( details.Capabilities );

  uint32_t image_count = details.Capabilities.minImageCount + 1;
  if ( details.Capabilities.maxImageCount > 0 &&
       image_count > details.Capabilities.maxImageCount ) {
    image_count = details.Capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapchain_info = {};
  swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_info.surface = m_SurfaceHandle->Get();
  swapchain_info.minImageCount = image_count;
  swapchain_info.imageFormat = surface_format.format;
  swapchain_info.imageColorSpace = surface_format.colorSpace;
  swapchain_info.presentMode = present_mode;
  swapchain_info.imageExtent = extent;
  swapchain_info.imageArrayLayers = 1;
  swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_info.preTransform = details.Capabilities.currentTransform;
  swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_info.clipped = true;

  QueueFamilyIndices queue_families = VulkanDevice::FindQueueFamilies( m_DeviceHandle->Physical(),
    m_SurfaceHandle->Get() );
  uint32_t indices[] = {
    queue_families.GraphicsFamily.value(),
    queue_families.PresentFamily.value()
  };

  if ( queue_families.GraphicsFamily != queue_families.PresentFamily ) {
    swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_info.queueFamilyIndexCount = 2;
    swapchain_info.pQueueFamilyIndices = indices;
  } else {
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  if ( VkResult res = vkCreateSwapchainKHR( m_DeviceHandle->Logical(), &swapchain_info, nullptr,
    &m_Swapchain); res != VK_SUCCESS ) {
    throw std::runtime_error( std::format( "failed to create swapchain! error code: {}", res ) );
  }

  vkGetSwapchainImagesKHR( m_DeviceHandle->Logical(), m_Swapchain, &image_count, nullptr );
  m_Images.resize( image_count );
  vkGetSwapchainImagesKHR( m_DeviceHandle->Logical(), m_Swapchain, &image_count, m_Images.data() );

  m_ImageFormat = surface_format.format;
  m_Extent = extent;


}

void VulkanSwapchain::Cleanup() {
  
}

void VulkanSwapchain::CreateImageView() {
  m_ImageViews.resize( m_Images.size() );
  for ( size_t i = 0; i < m_Images.size(); ++i ) {
    m_ImageViews[i] = 
  }
}

void VulkanSwapchain::Recreate() {
  int32_t width = 0, height = 0;
  glfwGetFramebufferSize( m_WindowHandle, &width, &height );

  while ( !width || !height ) {
    glfwGetFramebufferSize( m_WindowHandle, &width, &height );
    glfwWaitEvents();
  }

  vkDeviceWaitIdle( m_DeviceHandle->Logical() );

  Create( m_DeviceHandle, m_SurfaceHandle, m_WindowHandle );
  // Create image views
  // Create color resources
  // create depth resources
  // create framebuffers
}

SwapChainSupportDetails VulkanSwapchain::QuerySwapChainSupport(const VulkanDevice* device,
    const VulkanSurface* surface) {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->Physical(), surface->Get(),
    &details.Capabilities );

  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR( device->Physical(), surface->Get(),
    &format_count, nullptr );

  if ( format_count != 0 ) {
    details.Formats.resize( format_count );
    vkGetPhysicalDeviceSurfaceFormatsKHR( device->Physical(), surface->Get(),
      &format_count, details.Formats.data() );
  }

  uint32_t prmodes_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR( device->Physical(), surface->Get(),
    &prmodes_count, nullptr );

  if ( prmodes_count != 0 ) {
    details.PresentModes.resize( prmodes_count );
    vkGetPhysicalDeviceSurfacePresentModesKHR( device->Physical(),
      surface->Get(), &prmodes_count, details.PresentModes.data() );
  }

  return details;
}

VkImageView VulkanSwapchain::CreateImageView(VulkanDevice * device, VkImage image, VkFormat format,
  VkImageAspectFlags aspect_flags, uint32_t mip_levels ) {

  VkImageViewCreateInfo image_view_info = {};
  image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_info.image = image;
  image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_info.format = format;

  VkImageSubresourceRange subresource = {};
  subresource.aspectMask = aspect_flags;
  subresource.levelCount = mip_levels;
  subresource.layerCount = 1;

  image_view_info.subresourceRange = subresource;

  VkImageView image_view;
  if ( VkResult res = vkCreateImageView( device->Logical(), &image_view_info, nullptr,
    &image_view ); res != VK_SUCCESS ) {
    throw std::runtime_error(
      std::format( "failed to create texture image view! error code: {}", res )
    );
  }

  return image_view;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat( 
  const std::vector<VkSurfaceFormatKHR>& available ) const {

  for ( const auto& format : available) {
    if ( format.format == VK_FORMAT_B8G8R8A8_SRGB &&
         format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
      return format;
    }
  }

  return available[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(
  const std::vector<VkPresentModeKHR>& available ) const {

  for ( const auto& present_mode : available ) {
    if ( present_mode == VK_PRESENT_MODE_MAILBOX_KHR ) {
      return present_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(
  const VkSurfaceCapabilitiesKHR& capabilities ) const {
  if ( capabilities.currentExtent.width != UINT32_MAX ) {
    return capabilities.currentExtent;
  }

  int32_t width, height;
  glfwGetFramebufferSize( m_WindowHandle, &width, &height );

  VkExtent2D actual_extent = { STATIC_CAST( uint32_t, width ), STATIC_CAST( uint32_t, height ) };

  actual_extent.width = std::clamp(
    actual_extent.width,
    capabilities.minImageExtent.width,
    capabilities.maxImageExtent.width
  );

  actual_extent.height = std::clamp(
    actual_extent.height,
    capabilities.minImageExtent.height,
    capabilities.maxImageExtent.height
  );

  return actual_extent;
}

