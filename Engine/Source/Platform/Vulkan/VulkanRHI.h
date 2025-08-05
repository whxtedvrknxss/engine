// Source/Engine/Platform/Core/VulkanContext.h 

#ifndef __renderer_vulkan_context_h_included__
#define __renderer_vulkan_context_h_included__  

#include <vector>
#include <utility>
#include <optional>
#include <string>

#include <expected>

#include <vulkan/vulkan.h>

#include "Engine/RHI/RHI.h"
#include <span>

struct SDL_Window;

namespace VulkanRHI 
{

	template<typename VkType>
	using Expected = std::expected<VkType, std::string>;

	struct ContextCreateInfo
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
		VkDescriptorSetLayout DescriptorSetLayout;

		void inline Cleanup( VkDevice device, const VkAllocationCallbacks* alloc = nullptr )
		{
			vkDestroyDescriptorSetLayout( device, DescriptorSetLayout, alloc );
			vkDestroyPipeline( device, Instance, alloc );
			vkDestroyPipelineLayout( device, Layout, alloc );
			vkDestroyRenderPass( device, RenderPass, alloc );
		}
	};

	struct VulkanSyncObjects
	{
		VkSemaphore ImageAvailableSemaphores;
		VkSemaphore RenderFinishedSemaphores;
		VkFence InFlightFences;
	};

	struct VulkanBuffer
	{
		VkBuffer       Instance;
		VkDeviceMemory Memory;
		void* Mapped;

		void inline Cleanup( VkDevice device, const VkAllocationCallbacks* alloc = nullptr )
		{
			vkDestroyBuffer( device, Instance, alloc );
			vkFreeMemory( device, Memory, alloc );
		}
	};

	struct VulkanDescriptorGroup
	{
		VkDescriptorPool             Pool;
		std::vector<VkDescriptorSet> Sets;
	};

	struct VulkanTexture
	{
		VkImage        Image;
		VkImageView    View;
		VkSampler      Sampler=VK_NULL_HANDLE;
		VkDeviceMemory Memory;

		void inline Cleanup( VkDevice device, const VkAllocationCallbacks* alloc = nullptr )
		{
			if ( Sampler ) vkDestroySampler( device, Sampler, alloc );
			vkDestroyImageView( device, View, alloc );
			vkDestroyImage( device, Image, alloc );
			vkFreeMemory( device, Memory, alloc );
		}
	};

	class Context : public RHIContext
	{
	public:
		Context( ContextCreateInfo& context_info, SDL_Window* window );
		~Context();

		void Init() override;
		void Cleanup() override;

		void BeginFrame() override
		{
			( void ) 0;
		}

		void DrawFrame();

		void EndFrame() override
		{
			vkDeviceWaitIdle( Device );
		}

		void SwapBuffers() override
		{
			( void ) 0;
		}

	private:
		static bool IsExtensionAvailable( const std::vector<VkExtensionProperties>& props,
			const char* extension );
		static bool IsLayerAvailable( const std::vector<VkLayerProperties>& props, const char* layer );
		static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface );

	private:
		Expected<VkInstance>       CreateInstance();
		Expected<VkSurfaceKHR>     CreateSurface();
		Expected<VkPhysicalDevice> SelectPhysicalDevice();
		Expected<VkDevice>         CreateDevice( QueueFamilyIndices indices );
		VkQueue          GetQueue( uint32 family_index, uint32 index );

		Expected<VulkanSwapchain>  CreateSwapchain();
		void CleanupSwapchain();
		void RecreateSwapchain();

		VkShaderModule         CreateShaderModule( const std::vector<char>& code );
		Expected<VulkanGraphicsPipeline> CreateGraphicsPipeline( VkShaderModule vertex,
			VkShaderModule fragment );

		Expected<VkImageView> CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspect_flags );
		Expected<std::vector<VkImageView>>   CreateImageViews();
		Expected<std::vector<VkFramebuffer>> CreateFramebuffers();

		Expected<VkCommandPool> CreateCommandPool( QueueFamilyIndices indices );
		std::vector<VkCommandBuffer> CreateCommandBuffers();

		Expected<std::vector<VulkanSyncObjects>> CreateSyncObjects();

		uint32 FindMemoryType( uint32 type_filter, VkMemoryPropertyFlags prop_flags );

		Expected<VulkanBuffer> CreateBuffer( VkDeviceSize size, VkBufferUsageFlags usage,
			VkMemoryPropertyFlags props );
		Expected<VulkanBuffer> CreateVertexBuffer();
		Expected<VulkanBuffer> CreateIndexBuffer();
		Expected<std::vector<VulkanBuffer>> CreateUniformBuffers();
		void CopyBuffer( VkBuffer source, VkBuffer destination, VkDeviceSize size );

		void UpdateUniformBuffer( uint32 current_image );

		Expected<VulkanDescriptorGroup> CreateDescriptorGroup();

		Expected<VulkanTexture> CreateTexture();
		Expected<VulkanTexture> CreateTextureImage( int32 width, int32 height, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_props );

		Expected<VulkanTexture> CreateDepthTexture();

		void RecordCommandBuffer( uint32 image_index );

		Expected<VkCommandBuffer> BeginSingleTimeCommands();
		void EndSingleTimeCommands( VkCommandBuffer command_buffer );

		void TransitionImageLayout( const VulkanTexture& texture, VkFormat format,
			VkImageLayout old_layout, VkImageLayout new_layout );
		void CopyBufferToImage( VkBuffer buffer, VkImage image, uint32 width, uint32 height );

		Expected<VkFormat> FindSupportedFormat( std::span<const VkFormat> candidates,
			VkImageTiling tiling, VkFormatFeatureFlags features );
		Expected<VkFormat> FindDepthFormat();

	private:
		ContextCreateInfo ContextInfo;
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

		VulkanGraphicsPipeline GraphicsPipeline;

		VulkanSwapchain Swapchain;

		std::vector<VkImageView>   ImageViews;
		std::vector<VkFramebuffer> Framebuffers;

		std::vector<VulkanSyncObjects> SyncObjects;

		VulkanBuffer VertexBuffer;
		VulkanBuffer IndexBuffer;
		std::vector<VulkanBuffer> UniformBuffers;
		VulkanBuffer StagingBuffer;

		VulkanDescriptorGroup DescriptorGroup;

		VulkanTexture Texture;
		VulkanTexture DepthTexture;

	private:
		const int32 MAX_FRAMES_IN_FLIGHT = 2;
		uint32 CurrentFrame = 0;
	};

} // namespace VulkanRHI

#endif 

