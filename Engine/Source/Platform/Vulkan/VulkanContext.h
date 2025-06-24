// src/Engine/Renderer/Core/VulkanContext.h 

#ifndef __renderer_vulkan_context_h_included__
#define __renderer_vulkan_context_h_included__  

#include <string>
#include <optional>

#include "VulkanInstance.h"
#include "Engine/Renderer/GraphicsContext.h"

struct SDL_Window;

struct VulkanContextCreateInfo {
    int32 VulkanApiMajorVersion;
    int32 VulkanApiMinorVersion;
    std::vector<const char*> Extensions;
    std::vector<const char*> Layers;
    const char* ApplicationName;
    const char* EngineName;
};

struct QueueFamilyIndices {
    std::optional<uint32> Graphics;
    std::optional<uint32> Present;

    bool IsComplete() {
        return Graphics.has_value() && Present.has_value();
    }
};

class VulkanContext : public GraphicsContext {
public:
    VulkanContext( VulkanContextCreateInfo&& context_info, SDL_Window* window );
    void Init() override;
    void Cleanup() override;
    void BeginFrame() override {}
    void EndFrame() override {}
    void SwapBuffers() override {}

private:

    static void CheckVulkanResult( VkResult err );
    static bool IsExtensionAvailable( const std::vector<VkExtensionProperties>& props, const char* extension ); 
    static bool IsLayerAvailable( const std::vector<VkLayerProperties>& props, const char* layer );
    static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface );

private:
    VkInstance CreateInstance();
    VkSurfaceKHR CreateSurface();
    VkPhysicalDevice PickPhysicalDevice();
    VkDevice CreateDevice( QueueFamilyIndices indices );
    VkQueue  GetQueue( uint32 family_index, uint32 index );
    VkSwapchainKHR CreateSwapchain();

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
    VkSwapchainKHR   Swapchain;
};

#endif 

