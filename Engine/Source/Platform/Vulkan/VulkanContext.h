// Source/Engine/Platform/Core/VulkanContext.h 

#ifndef __renderer_vulkan_context_h_included__
#define __renderer_vulkan_context_h_included__  

#include <vector>
#include <utility>
#include <optional>

#include <vulkan/vulkan.h>

#include "Engine/Renderer/GraphicsContext.h"

struct SDL_Window;

struct VulkanContextCreateInfo
{
    int32 ApiMajorVersion;
    int32 ApiMinorVersion;
    std::vector<const char*> Extensions;
    std::vector<const char*> Layers;
    const char* ApplicationName;
    const char* EngineName;
};

struct QueueFamilyIndices
{
    std::optional<uint32> Graphics;
    std::optional<uint32> Present;

    bool IsComplete() const
    {
        return Graphics.has_value() && Present.has_value();
    }
};

struct VulkanSwapchain
{
    VkSwapchainKHR Instance;
    VkFormat       Format;
    VkExtent2D     Extent;
    std::vector<VkImage> Images;
};

struct VulkanGraphicsPipeline
{
    VkRenderPass     RenderPass;
    VkPipelineLayout Layout;
    VkPipeline       Instance;
};

struct VulkanSyncObjects
{
    std::vector<VkSemaphore> ImageAvailableSemaphores;
    std::vector<VkSemaphore> RenderFinishedSemaphores;
    std::vector<VkFence> InFlightFences;
};

struct VulkanBuffer
{
    VkBuffer       Instance;
    VkDeviceMemory Memory;
};

class VulkanContext : public GraphicsContext
{
public:
    VulkanContext( VulkanContextCreateInfo& context_info, SDL_Window* window );
    ~VulkanContext();

    void Init() override;
    void Cleanup() override;

    void BeginFrame() override
    {
    }

    void DrawFrame();

    void EndFrame() override 
    {
        vkDeviceWaitIdle( Device );
    }

    void SwapBuffers() override
    {
    }

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
    void CleanupSwapchain();
    void RecreateSwapchain();

    VkShaderModule         CreateShaderModule( const std::vector<char>& code );
    VulkanGraphicsPipeline CreateGraphicsPipeline( VkShaderModule vertex, VkShaderModule fragment );

    std::vector<VkImageView>   CreateImageViews();
    std::vector<VkFramebuffer> CreateFramebuffers();

    VkCommandPool CreateCommandPool( QueueFamilyIndices indices );
    std::vector<VkCommandBuffer> CreateCommandBuffers();

    VulkanSyncObjects CreateSyncObjects();

	VulkanBuffer CreateBuffer( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props );
    VulkanBuffer CreateVertexBuffer();
    VulkanBuffer CreateIndexBuffer();

    void CopyBuffer( VkBuffer source, VkBuffer destination, VkDeviceSize size );
    
private:
    void RecordCommandBuffer( uint32 image_index );

private:
    VulkanContextCreateInfo ContextInfo;
    SDL_Window* WindowHandle;

private:
    VkInstance       Instance;
    VkSurfaceKHR     Surface;
    VkPhysicalDevice PhysicalDevice;
    VkDevice         Device;
    VkQueue          GraphicsQueue;
    VkQueue          PresentQueue;

    VkCommandPool   CommandPool;
    std::vector<VkCommandBuffer> CommandBuffers;

    //  pipeline related 
    VulkanGraphicsPipeline GraphicsPipeline;

    // swapchain related
    VulkanSwapchain Swapchain;

    std::vector<VkImageView>   ImageViews;
    std::vector<VkFramebuffer> Framebuffers;

    VulkanSyncObjects SyncObjects;

    VulkanBuffer VertexBuffer;
    VulkanBuffer IndexBuffer;

private:
    const int32 MAX_FRAMES_IN_FLIGHT = 2;
    uint32 CurrentFrame = 0;
};

#endif 

