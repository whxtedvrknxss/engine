// src/Platform/Vulkan/VulkanSwapchain.h 

#ifndef __renderer_vulkan_swapchain_h_included__
#define __renderer_vulkan_swapchain_h_included__

#include <vulkan/vulkan.h>

#include "VulkanDevice.h"
#include "VulkanSurface.h"

class VulkanSwapchain {
public:
    struct Features {
        VkSurfaceCapabilitiesKHR        Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR>   PresentModes;
    };

    void Create( VulkanDevice* device, VulkanSurface* surface, SDL_Window* window );
    void Cleanup();

    void Recreate();

    static Features QuerySwapChainSupport( const VulkanDevice* device,
        const VulkanSurface* surface );

private:
    void CreateImageViews();
    static VkImageView CreateImageView( VulkanDevice* device, VkImage image, VkFormat format,
        VkImageAspectFlags aspect_flags, uint32_t mip_levels );

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& available ) const;
    VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& available ) const;
    VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) const;

private:
    VulkanDevice* DeviceHandle = nullptr;
    VulkanSurface* SurfaceHandle = nullptr;
    SDL_Window* WindowHandle = nullptr;

private:
    VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
    VkExtent2D Extent = {};

    std::vector<VkImage> Images;
    std::vector<VkImageView> ImageViews;
    VkFormat ImageFormat = VK_FORMAT_UNDEFINED;

    std::vector<VkFramebuffer> Framebuffers;

};

#endif

