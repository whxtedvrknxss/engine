#include "VulkanSwapchain.h"

void VulkanSwapchain::Create(VulkanDevice* device, VulkanSurface* surface) {
  m_DeviceHandle = device;
  m_SurfaceHandle = surface;

  SwapChainSupportDetails details = QuerySwapChainSupport();

  VkSurfaceFormatKHR 
}

void VulkanSwapchain::Cleanup() {}

void VulkanSwapchain::Recreate() {}

SwapChainSupportDetails VulkanSwapchain::QuerySwapChainSupport() const {
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( m_DeviceHandle->GetPhysical(), m_SurfaceHandle->Get(),
    &details.Capabilities );

  uint32_t format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR( m_DeviceHandle->GetPhysical(), m_SurfaceHandle->Get(),
    &format_count, nullptr );

  if ( format_count != 0 ) {
    details.Formats.resize( format_count );
    vkGetPhysicalDeviceSurfaceFormatsKHR( m_DeviceHandle->GetPhysical(), m_SurfaceHandle->Get(),
      &format_count, details.Formats.data() );
  }

  uint32_t prmodes_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR( m_DeviceHandle->GetPhysical(), m_SurfaceHandle->Get(),
    &prmodes_count, nullptr );

  if ( prmodes_count != 0 ) {
    details.PresentModes.resize( prmodes_count );
    vkGetPhysicalDeviceSurfacePresentModesKHR( m_DeviceHandle->GetPhysical(),
      m_SurfaceHandle->Get(), &prmodes_count, details.PresentModes.data() );
  }

  return details;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat() {
  for ( const auto& available_format : m_Details.Formats ) {
    if ( available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
         available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
      return available_format;
    }
  }

  return m_Details.Formats[0];
}
