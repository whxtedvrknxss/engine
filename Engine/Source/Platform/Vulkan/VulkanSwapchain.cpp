#include "VulkanSwapchain.h"

#include <algorithm>
#include <format>
#include <stack>
#include <stdexcept>

#include <SDl3/SDL_events.h>

#include "Engine/Core/Common.h"

void VulkanSwapchain::Create( VulkanDevice* device, VulkanSurface* surface,
    SDL_Window* window ) {
    DeviceHandle = device;
    SurfaceHandle = surface;

    Features details = QuerySwapChainSupport( device, surface );

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
    swapchain_info.surface = SurfaceHandle->Get();
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

    //QueueFamiliesIndices queue_families = VulkanDevice::FindQueueFamilies(
     //   DeviceHandle->GetPhysical() );

    if ( VkResult res = vkCreateSwapchainKHR( DeviceHandle->GetLogical(), &swapchain_info, nullptr,
        &Swapchain ); res != VK_SUCCESS ) {
        throw std::runtime_error(  "failed to create swapchain! error code: " );
    }

    vkGetSwapchainImagesKHR( DeviceHandle->GetLogical(), Swapchain, &image_count, nullptr );
    Images.resize( image_count );
    vkGetSwapchainImagesKHR( DeviceHandle->GetLogical(), Swapchain, &image_count, Images.data() );

    ImageFormat = surface_format.format;
    Extent = extent;


}

void VulkanSwapchain::Cleanup() {


    for ( auto& image_view : ImageViews ) {
        vkDestroyImageView( DeviceHandle->GetLogical(), image_view, nullptr );
    }

    vkDestroySwapchainKHR( DeviceHandle->GetLogical(), Swapchain, nullptr );
}

void VulkanSwapchain::Recreate() {
    int32_t width = 0, height = 0;
    SDL_GetWindowSizeInPixels( WindowHandle, &width, &height );

    SDL_Event event;
    while ( !width || !height ) {
        SDL_GetWindowSizeInPixels( WindowHandle, &width, &height );
        SDL_WaitEvent( &event );
    }

    vkDeviceWaitIdle( DeviceHandle->GetLogical() );

    Create( DeviceHandle, SurfaceHandle, WindowHandle );
    CreateImageViews();
    // Create color resources
    // create depth resources
    // create framebuffers
}

VulkanSwapchain::Features VulkanSwapchain::QuerySwapChainSupport( const VulkanDevice* device,
    const VulkanSurface* surface ) {
    Features features;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->GetPhysical(), surface->Get(),
        &features.Capabilities );

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device->GetPhysical(), surface->Get(),
        &format_count, nullptr );

    if ( format_count != 0 ) {
        features.Formats.resize( format_count );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device->GetPhysical(), surface->Get(),
            &format_count, features.Formats.data() );
    }

    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device->GetPhysical(), surface->Get(),
        &present_modes_count, nullptr );

    if ( present_modes_count != 0 ) {
        features.PresentModes.resize( present_modes_count );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device->GetPhysical(),
            surface->Get(), &present_modes_count, features.PresentModes.data() );
    }

    return features;
}

void VulkanSwapchain::CreateImageViews() {
    ImageViews.resize( Images.size() );
    for ( size_t i = 0; i < Images.size(); i++ ) {
        ImageViews[i] = CreateImageView( DeviceHandle, Images[i], ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
    }
}

VkImageView VulkanSwapchain::CreateImageView( VulkanDevice* device, VkImage image, VkFormat format,
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
    if ( VkResult res = vkCreateImageView( device->GetLogical(), &image_view_info, nullptr,
        &image_view ); res != VK_SUCCESS ) {
        throw std::runtime_error(
            "failed to create texture image view! error code "
        );
    }

    return image_view;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& available ) const {

    for ( const auto& format : available ) {
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
    SDL_GetWindowSizeInPixels( WindowHandle, &width, &height );

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

