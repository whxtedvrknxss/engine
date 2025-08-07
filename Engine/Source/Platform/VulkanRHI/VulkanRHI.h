// Source/Engine/Platform/Core/VulkanContext.h 

#pragma once

#include <span>
#include <vector>
#include <string>
#include <utility>
#include <optional>
#include <expected>

#include <vulkan/vulkan.h>

#include "Engine/RHI/RHI.h"

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

namespace VulkanRHI 
{

	template<typename VkType>
	using Expected = std::expected<VkType, std::string>;

	struct VulkanQueueFamilyIndices
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
		VkSwapchainKHR Instance = VK_NULL_HANDLE;
		VkFormat       Format = VK_FORMAT_UNDEFINED;
		VkExtent2D     Extent = {};
		std::vector<VkImage> Images;
		std::vector<VkImageView> ImageViews;
		std::vector<VkFramebuffer> Framebuffers;

		void Destroy( VkDevice device, const VkAllocationCallbacks* alloc = nullptr )
		{
			for ( auto framebuffer : Framebuffers )
			{
				vkDestroyFramebuffer( device, framebuffer, alloc );
			}

			for ( auto image_view : ImageViews )
			{
				vkDestroyImageView( device, image_view, alloc );
			}

			vkDestroySwapchainKHR( device, Instance, alloc );
		}
	};

	struct VulkanGraphicsPipeline
	{
		VkRenderPass     RenderPass = VK_NULL_HANDLE;
		VkPipelineLayout Layout = VK_NULL_HANDLE;
		VkPipeline       Instance = VK_NULL_HANDLE;
		VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;

		void inline Destroy( VkDevice device, const VkAllocationCallbacks* alloc = nullptr )
		{
			vkDestroyDescriptorSetLayout( device, DescriptorSetLayout, alloc );
			vkDestroyPipeline( device, Instance, alloc );
			vkDestroyPipelineLayout( device, Layout, alloc );
			vkDestroyRenderPass( device, RenderPass, alloc );
		}
	};

	struct VulkanSyncObjects
	{
		VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
		VkFence InFlightFence = VK_NULL_HANDLE;

		void inline Destroy( VkDevice device, const VkAllocationCallbacks* alloc = nullptr )
		{
			vkDestroySemaphore( device, ImageAvailableSemaphore, alloc );
			vkDestroySemaphore( device, RenderFinishedSemaphore, alloc );
			vkDestroyFence( device, InFlightFence, alloc );
		}
	};

	struct VulkanBuffer
	{
		VkBuffer       Instance = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		void*		   Mapped = nullptr;

		void inline Destroy( VkDevice device, const VkAllocationCallbacks* alloc = nullptr )
		{
			vkDestroyBuffer( device, Instance, alloc );
			vkFreeMemory( device, Memory, alloc );
		}
	};

	struct VulkanDescriptorGroup
	{
		VkDescriptorPool             Pool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> Sets;
	};

	struct VulkanTexture
	{
		VkImage        Image = VK_NULL_HANDLE;
		VkImageView    View = VK_NULL_HANDLE;
		VkSampler      Sampler = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;

		void inline Destroy( VkDevice device, const VkAllocationCallbacks* alloc = nullptr )
		{
			if ( Sampler )
			{
				vkDestroySampler( device, Sampler, alloc );
			}
			vkDestroyImageView( device, View, alloc );
			vkDestroyImage( device, Image, alloc );
			vkFreeMemory( device, Memory, alloc );
		}
	};

	class Context : public RHIContext
	{
	public:
		Context( VulkanContextCreateInfo& context_info, SDL_Window* window );
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
			//vkDeviceWaitIdle( Device );
		}

		void SwapBuffers() override
		{
			( void ) 0;
		}

	private:
		static bool IsExtensionAvailable( const std::vector<VkExtensionProperties>& props,
			const char* extension );
		static bool IsLayerAvailable( const std::vector<VkLayerProperties>& props, const char* layer );

		static Expected<VulkanQueueFamilyIndices> FindQueueFamilies( VkPhysicalDevice device, 
			VkSurfaceKHR surface );

	private:
		Expected<VkInstance>       CreateInstance();
		Expected<VkSurfaceKHR>     CreateSurface();
		Expected<VkPhysicalDevice> SelectPhysicalDevice();
		Expected<VkDevice>         CreateDevice( VulkanQueueFamilyIndices indices );
		VkQueue          GetQueue( uint32 family_index, uint32 index );

		Expected<VulkanSwapchain>  CreateSwapchain();
		void RecreateSwapchain();

		Expected<VkShaderModule>         CreateShaderModule( const std::vector<char>& code );
		Expected<VulkanGraphicsPipeline> CreateGraphicsPipeline( VkShaderModule vertex,
			VkShaderModule fragment );

		Expected<VkImageView> CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspect_flags );
		Expected<std::vector<VkImageView>>   CreateImageViews();
		Expected<std::vector<VkFramebuffer>> CreateFramebuffers();

		Expected<VkCommandPool>				   CreateCommandPool( VulkanQueueFamilyIndices indices );
		Expected<std::vector<VkCommandBuffer>> CreateCommandBuffers();

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

		void TransitionImageLayout( const VulkanTexture& texture, VkFormat format,VkImageLayout old_layout,
			VkImageLayout new_layout );
		void CopyBufferToImage( VkBuffer buffer, VkImage image, uint32 width, uint32 height );

		Expected<VkFormat> FindSupportedFormat( std::span<const VkFormat> candidates,
			VkImageTiling tiling, VkFormatFeatureFlags features ) const;
		Expected<VkFormat> FindDepthFormat();

	private:
		VulkanContextCreateInfo ContextInfo;
		SDL_Window* WindowHandle;

	private:
		VkInstance       Instance;
		VkSurfaceKHR     Surface;
		VkPhysicalDevice Gpu;
		VkDevice         Device;
		VkQueue          GraphicsQueue;
		VkQueue          PresentQueue;

		VkCommandPool   CommandPool;
		std::vector<VkCommandBuffer> CommandBuffers;

		VulkanGraphicsPipeline GraphicsPipeline;

		VulkanSwapchain Swapchain;

		std::vector<VulkanSyncObjects> SyncObjects;

		VulkanBuffer VertexBuffer;
		VulkanBuffer IndexBuffer;
		std::vector<VulkanBuffer> UniformBuffers;

		VulkanDescriptorGroup DescriptorGroup;

		VulkanTexture Texture;
		VulkanTexture DepthTexture;

	private:
		const int32 MAX_FRAMES_IN_FLIGHT = 2;
		uint32 CurrentFrame = 0;
	};

} // namespace VulkanRHI


