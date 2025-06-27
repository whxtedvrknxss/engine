// Source/Engine/Platform/Core/VulkanContext.h 

#ifndef __renderer_vulkan_context_h_included__
#define __renderer_vulkan_context_h_included__  

#include <vector>
#include <utility>
#include <optional>

#include <vulkan/vulkan.h>

#include "Engine/Renderer/GraphicsContext.h"

struct SDL_Window;

struct VulkanContextCreateInfo {
    int32 ApiMajorVersion;
    int32 ApiMinorVersion;
    std::vector<const char*> Extensions; 
    std::vector<const char*> Layers;
    const char* ApplicationName;
    const char* EngineName;
};

struct QueueFamilyIndices {
    std::optional<uint32> Graphics;
    std::optional<uint32> Present;

    bool IsComplete() const {
        return Graphics.has_value() && Present.has_value();
    }
};

struct VulkanSwapchain {
    VkSwapchainKHR Handle;
    VkFormat       Format;
    VkExtent2D     Extent;
    std::vector<VkImage> Images;

    int8 a;
};

class VulkanContext : public GraphicsContext {
public:
    VulkanContext( VulkanContextCreateInfo& context_info, SDL_Window* window );
    void Init() override;
    void Cleanup() override;
    void BeginFrame() override {}
    void EndFrame() override {}
    void SwapBuffers() override {}

private:
    static bool IsExtensionAvailable( const std::vector<VkExtensionProperties>& props, const char* extension ); 
    static bool IsLayerAvailable( const std::vector<VkLayerProperties>& props, const char* layer );
    static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface );

private:
    VkInstance       CreateInstance();
    VkSurfaceKHR     CreateSurface();
    VkPhysicalDevice SelectPhysicalDevice();
    VkDevice         CreateDevice( QueueFamilyIndices indices );
    VkQueue          GetQueue( uint32 family_index, uint32 index );
    VulkanSwapchain  CreateSwapchain(); 
    std::vector<VkImageView> CreateImageViews();
    VkShaderModule   CreateShaderModule( const std::vector<char>& code );
    void CreateGraphicsPipeline( VkShaderModule vertex, VkShaderModule fragment );

private:
    VulkanContextCreateInfo ContextInfo;
    SDL_Window*             WindowHandle;

private:
    VkInstance       Instance;
    VkSurfaceKHR     Surface;
    VkPhysicalDevice PhysicalDevice;
    VkDevice         Device;
    VkQueue          GraphicsQueue;
    VkQueue          PresentQueue;
    VkPipeline       GraphicsPipeline;

    // swapchain related
    VulkanSwapchain Swapchain;
    
    std::vector<VkImageView> ImageViews;
};

#endif 

