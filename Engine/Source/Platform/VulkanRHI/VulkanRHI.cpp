#include "VulkanRHI.h"

#include <set>
#include <chrono>
#include <stdexcept>
#include <expected>

#include <SDL3/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Engine/Core/Common.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Application.h"
#include "Shader.h"
#include "VulkanMath.h"

namespace VulkanRHI
{

	Context::Context( VulkanContextCreateInfo& context_info, SDL_Window* window_handle )
	{
		ContextInfo = std::move( context_info );
		WindowHandle = window_handle;

		Instance = VK_NULL_HANDLE;
		Surface = VK_NULL_HANDLE;
		PhysicalDevice = VK_NULL_HANDLE;
		Device = VK_NULL_HANDLE;
		GraphicsQueue = VK_NULL_HANDLE;
		PresentQueue = VK_NULL_HANDLE;
		CommandPool = VK_NULL_HANDLE;
	}

	Context::~Context()
	{
		Cleanup();
	}

	void Context::Init()
	{
		auto instance_result = CreateInstance();
		if ( !instance_result )
		{
			LOG_ERROR( "{}", instance_result.error() );
			throw std::runtime_error( "Instance == VK_NULL_HANDLE" );
		}
		Instance = std::move( instance_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Instance." );

		auto surface_result = CreateSurface();
		if ( !surface_result )
		{
			LOG_ERROR( "{}", surface_result.error() );
			throw std::runtime_error( "Surface == VK_NULL_HANDLE" );
		}
		Surface = std::move( surface_result.value() );
		LOG_INFO( "[Vulkan] Successfully created surface." );

		auto physical_device_result = SelectPhysicalDevice();
		if ( !physical_device_result )
		{
			LOG_ERROR( "{}", physical_device_result.error() );
			throw std::runtime_error( "PhysicalDevice == VK_NULL_HANDLE" );
		}
		PhysicalDevice = std::move( physical_device_result.value() );
		LOG_INFO( "[Vulkan] Successfully selected Physical Device." );

		auto indices_result = FindQueueFamilies( PhysicalDevice, Surface );
		if ( !indices_result )
		{
			LOG_ERROR( "{}", indices_result.error() );
			throw std::runtime_error( "queue families invalid" );
		}
		VulkanQueueFamilyIndices indices = std::move( indices_result.value() );
		LOG_INFO( "[Vulkan] Queue family indices are: GRAPHICS = {}, PRESENT = {}.", indices.Graphics.value(),
			indices.Present.value() );

		auto device_result = CreateDevice( indices );
		if ( !device_result )
		{
			LOG_ERROR( "{}", device_result.error() );
			throw std::runtime_error( "Device == VK_NULL_HANDLE" );
		}
		Device = std::move( device_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Device." );

		GraphicsQueue = GetQueue( indices.Graphics.value(), 0 );
		if ( GraphicsQueue == VK_NULL_HANDLE )
		{
			throw std::runtime_error( "GraphicsQueue == VK_NULL_HANDLE" );
		}

		PresentQueue = GetQueue( indices.Present.value(), 0 );
		if ( PresentQueue == VK_NULL_HANDLE )
		{
			throw std::runtime_error( "PresentQueue == VK_NULL_HANDLE" );
		}

		auto swapchain_result = CreateSwapchain();
		if ( !swapchain_result )
		{
			LOG_ERROR( "{}", swapchain_result.error() );
			throw std::runtime_error( "Swapchain == VK_NULL_HANDLE || SwapchainImages.size == 0" );
		}
		Swapchain = std::move( swapchain_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Swapchain." );

		auto image_views_result = CreateImageViews();
		if ( !image_views_result )
		{
			LOG_ERROR( "{}", image_views_result.error() );
			throw std::runtime_error( "ImageViews == null" );
		}
		Swapchain.ImageViews = std::move( image_views_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Image Views" );

		auto shaders_path = std::filesystem::current_path().parent_path() / "Engine" / "Shaders";
		auto vertex_shader = GetShaderSource( shaders_path / "triangle.vert.spv" );
		auto fragment_shader = GetShaderSource( shaders_path / "triangle.frag.spv" );

		VkShaderModule vertex = CreateShaderModule( vertex_shader );
		if ( vertex == VK_NULL_HANDLE )
		{
			throw std::runtime_error( "vertex == VK_NULL_HANDLE" );
		}

		VkShaderModule fragment = CreateShaderModule( fragment_shader );
		if ( fragment == VK_NULL_HANDLE )
		{
			throw std::runtime_error( "fragment == VK_NULL_HANDLE" );
		}

		auto graphics_pipeline_result = CreateGraphicsPipeline( vertex, fragment );
		if ( !graphics_pipeline_result )
		{
			LOG_ERROR( "{}", graphics_pipeline_result.error() );
			throw std::runtime_error( "GraphicsPipeline == VK_NULL_HANDLE" );
		}
		GraphicsPipeline = std::move( graphics_pipeline_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Graphics Pipeline." );

		vkDestroyShaderModule( Device, vertex, nullptr );
		vkDestroyShaderModule( Device, fragment, nullptr );

		auto command_pool_result = CreateCommandPool( indices );
		if ( !command_pool_result )
		{
			LOG_ERROR( "{}", command_pool_result.error() );
			throw std::runtime_error( "CommandPool == VK_NULL_HANDLE" );
		}
		CommandPool = std::move( command_pool_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Command Pool." );

		CommandBuffers = CreateCommandBuffers();
		for ( int32 i = 0; i < CommandBuffers.size(); ++i )
		{
			if ( CommandBuffers[i] == VK_NULL_HANDLE )
			{
				throw std::runtime_error( "CommandBuffer == VK_NULL_HANDLE" );
			}
		}

		auto sync_objects_result = CreateSyncObjects();
		if ( !sync_objects_result )
		{
			LOG_ERROR( "{}", sync_objects_result.error() );
			throw std::runtime_error( "synchronization objects are invalid" );
		}
		SyncObjects = std::move( sync_objects_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Synchronization objects." );

		auto texture_result = CreateTexture();
		if ( !texture_result )
		{
			LOG_ERROR( "{}", texture_result.error() );
			throw std::runtime_error( "texture image == VK_NULL_HANDLE" );
		}
		Texture = std::move( texture_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Texture" );

		auto depth_texture_result = CreateDepthTexture();
		if ( !depth_texture_result )
		{
			LOG_ERROR( "{}", depth_texture_result.error() );
			throw std::runtime_error( "depth texture == null" );
		}
		DepthTexture = std::move( depth_texture_result.value() );
		LOG_INFO( "[Vulkan] Successfully created depth texture." );

		auto framebuffers_result = CreateFramebuffers();
		if ( !framebuffers_result )
		{
			LOG_ERROR( "{}", framebuffers_result.error() );
			throw std::runtime_error( "Framebuffers == VK_NULL_HANDLE" );
		}
		Swapchain.Framebuffers = std::move( framebuffers_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Vulkan Framebuffers" );

		auto vertex_buffer_result = CreateVertexBuffer();
		if ( !vertex_buffer_result )
		{
			LOG_ERROR( "{}", vertex_buffer_result.error() );
			throw std::runtime_error( "VertexBuffer == VK_NULL_HANDLE || VertexBufferMemory == VK_NULL_HANDLE" );
		}
		VertexBuffer = std::move( vertex_buffer_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Vertex Buffer." );

		auto index_buffer_result = CreateIndexBuffer();
		if ( !index_buffer_result )
		{
			LOG_ERROR( "{}", index_buffer_result.error() );
			throw std::runtime_error( "IndexBuffer == VK_NULL_HANDLE || IndexBufferMemory == VK_NULL_HANDLE" );
		}
		IndexBuffer = std::move( index_buffer_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Index Buffer." );

		auto uniform_buffers_result = CreateUniformBuffers();
		if ( !uniform_buffers_result )
		{
			LOG_ERROR( "{}", uniform_buffers_result.error() );
			throw std::runtime_error( "UniformBuffers == VK_NULL_HANDLE" );
		}
		UniformBuffers = std::move( uniform_buffers_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Uniform Buffers." );

		auto descriptor_group_result = CreateDescriptorGroup();
		if ( !descriptor_group_result )
		{
			LOG_ERROR( "{}", descriptor_group_result.error() );
			throw std::runtime_error( "DescriptorPool == VK_NULL_HANDLE" );
		}
		DescriptorGroup = std::move( descriptor_group_result.value() );
		LOG_INFO( "[Vulkan] Successfully created Descriptor group." );
	}

	void Context::Cleanup()
	{
		const VkAllocationCallbacks* alloc = nullptr;

		vkDeviceWaitIdle( Device );

		DepthTexture.Cleanup( Device );
		Swapchain.Cleanup( Device );

		Texture.Cleanup( Device );

		vkDestroyDescriptorPool( Device, DescriptorGroup.Pool, alloc );

		for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			UniformBuffers[i].Cleanup( Device );
		}

		IndexBuffer.Cleanup( Device );
		VertexBuffer.Cleanup( Device );

		GraphicsPipeline.Cleanup( Device );

		for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			vkDestroySemaphore( Device, SyncObjects[i].ImageAvailableSemaphores, alloc );
			vkDestroySemaphore( Device, SyncObjects[i].RenderFinishedSemaphores, alloc );
			vkDestroyFence( Device, SyncObjects[i].InFlightFences, alloc );
		}

		vkDestroyCommandPool( Device, CommandPool, alloc );

		vkDestroyDevice( Device, alloc );
		vkDestroySurfaceKHR( Instance, Surface, alloc );
		vkDestroyInstance( Instance, alloc );
	}

	void Context::DrawFrame()
	{
		VkResult err;

		const uint32   fence_count = 1;
		const VkBool32 wait_all = VK_TRUE;
		const uint64   timeout = UINT64_MAX;
		err = vkWaitForFences( Device, fence_count, &SyncObjects[CurrentFrame].InFlightFences, wait_all,
			timeout );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR( "[Vulkan] Error from vkWaitForFences: {}.", err );
		}

		uint32  image_index;
		VkFence FENCE = VK_NULL_HANDLE;
		err = vkAcquireNextImageKHR( Device, Swapchain.Instance, timeout,
			SyncObjects[CurrentFrame].ImageAvailableSemaphores, FENCE, &image_index );
		if ( err == VK_ERROR_OUT_OF_DATE_KHR )
		{
			RecreateSwapchain();
			return;
		}
		else if ( err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR )
		{
			LOG_ERROR( "[Vulkan] Error from vkAcquireNextImageKHR: {}.", err );
		}

		UpdateUniformBuffer( CurrentFrame );

		err = vkResetFences( Device, fence_count, &SyncObjects[CurrentFrame].InFlightFences );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR( "[Vulkan] Error from vkResetFences: {}.", err );
		}

		err = vkResetCommandBuffer( CommandBuffers[CurrentFrame], 0 );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR( "[Vulkan] Error from vkResetCommandBuffer: {}.", err  );
		}
		RecordCommandBuffer( image_index );

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		std::array<VkSemaphore, 1> wait_semaphores = { SyncObjects[CurrentFrame].ImageAvailableSemaphores };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.waitSemaphoreCount = static_cast<uint32>( wait_semaphores.size() );
		submit_info.pWaitSemaphores = wait_semaphores.data();
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &CommandBuffers[CurrentFrame];

		std::array<VkSemaphore, 1> signal_semaphores = { SyncObjects[CurrentFrame].RenderFinishedSemaphores };
		submit_info.signalSemaphoreCount = static_cast<uint32>( signal_semaphores.size() );
		submit_info.pSignalSemaphores = signal_semaphores.data();

		const uint32 submit_count = 1;
		err = vkQueueSubmit( GraphicsQueue, submit_count, &submit_info, 
			SyncObjects[CurrentFrame].InFlightFences );
		if ( err != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to submit draw command buffer!" );
		}

		VkPresentInfoKHR present_info = {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = static_cast<uint32>( signal_semaphores.size() );
		present_info.pWaitSemaphores = signal_semaphores.data();

		std::array<VkSwapchainKHR, 1> swapchains = { Swapchain.Instance };
		present_info.swapchainCount = static_cast<uint32>( swapchains.size() );
		present_info.pSwapchains = swapchains.data();
		present_info.pImageIndices = &image_index;

		err = vkQueuePresentKHR( PresentQueue, &present_info );
		if ( err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR )
		{
			RecreateSwapchain();
		}
		else if ( err != VK_SUCCESS )
		{

		}

		CurrentFrame = ( CurrentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
	}

	bool Context::IsExtensionAvailable( const std::vector<VkExtensionProperties>& props,
		const char* extension )
	{
		for ( const auto& prop : props )
		{
			if ( strcmp( prop.extensionName, extension ) == 0 )
			{
				return true;
			}
		}
		return false;
	}

	bool Context::IsLayerAvailable( const std::vector<VkLayerProperties>& props, const char* layer )
	{
		for ( const auto& prop : props )
		{
			if ( strcmp( prop.layerName, layer ) == 0 )
			{
				return true;
			}
		}
		return false;
	}

	Expected<VkInstance> Context::CreateInstance()
	{
		VkResult err;

		VkApplicationInfo app_info = {};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = ContextInfo.ApplicationName;
		app_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 0 );
		app_info.pEngineName = ContextInfo.EngineName;
		app_info.engineVersion = VK_MAKE_VERSION( 0, 0, 0 );
		app_info.apiVersion = VK_MAKE_API_VERSION( 0, ContextInfo.ApiMajorVersion,
			ContextInfo.ApiMinorVersion, 0 );

		VkInstanceCreateInfo instance_info = {};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app_info;

		uint32 count_extensions = 0;
		err = vkEnumerateInstanceExtensionProperties( nullptr, &count_extensions, nullptr );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to enumerate Vulkan Instance extension properties. "
				"vkEnumerateInstanceExtensionProperties returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		std::vector<VkExtensionProperties> available_extensions( count_extensions );
		err = vkEnumerateInstanceExtensionProperties( nullptr, &count_extensions, available_extensions.data() );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to enumerate Vulkan Instance extension properties. "
				"vkEnumerateInstanceExtensionProperties returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		for ( const auto& ext : ContextInfo.Extensions )
		{
			if ( !IsExtensionAvailable( available_extensions, ext ) )
			{
				std::string message = std::format(
					"[Vulkan] Required extension is not available. Extension name: {}",
					ext );
				return std::unexpected( message );
			}
		}

		instance_info.enabledExtensionCount = static_cast< uint32 >( ContextInfo.Extensions.size() );
		instance_info.ppEnabledExtensionNames = ContextInfo.Extensions.data();

		uint32 count_layers = 0;
		err = vkEnumerateInstanceLayerProperties( &count_layers, nullptr );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to enumerate Vulkan Instance layer properties. "
				"vkEnumerateInstanceLayerProperties() returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		std::vector<VkLayerProperties> available_layers( count_layers );
		err = vkEnumerateInstanceLayerProperties( &count_layers, available_layers.data() );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to enumerate Vulkan Instance layer properties. "
				"vkEnumerateInstanceLayerProperties() returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		for ( const auto& layer : ContextInfo.Layers )
		{
			if ( !IsLayerAvailable( available_layers, layer ) )
			{
				std::string message = std::format(
					"Required layer is not available. Layer name: %s",
					layer );
				return std::unexpected( message );
			}
		}

		instance_info.enabledLayerCount = static_cast< uint32 >( ContextInfo.Layers.size() );
		instance_info.ppEnabledLayerNames = ContextInfo.Layers.data();

		VkInstance instance;
		err = vkCreateInstance( &instance_info, nullptr, &instance );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Instance. vkCreateInstance() returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		return instance;
	}

	Expected<VkSurfaceKHR> Context::CreateSurface()
	{
		VkSurfaceKHR surface;
		if ( !SDL_Vulkan_CreateSurface( WindowHandle, Instance, nullptr, &surface ) )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Surface with SDL_Vulkan_CreateSurface. "
				"Error message : %s",
				SDL_GetError() );
			return std::unexpected( message );
		}
		return surface;
	}

	Expected<VkPhysicalDevice> Context::SelectPhysicalDevice()
	{
		VkResult err;

		uint32 count = 0;
		err = vkEnumeratePhysicalDevices( Instance, &count, nullptr );
		if ( err != VK_SUCCESS || count == 0 )
		{
			std::string message = std::format(
				"[Vulkan] Failed to enumerate GPUs with Vulkan support."
				" vkEnumeratePhysicalDevices returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		std::vector<VkPhysicalDevice> devices( count );
		err = vkEnumeratePhysicalDevices( Instance, &count, devices.data() );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to enumerate GPUs with Vulkan support."
				" vkEnumeratePhysicalDevices returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		for ( const auto& device : devices )
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties( device, &props );

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures( device, &features );

			//QueueFamilyIndices indices = FindQueueFamilies( device );

			//uint32 count_extensions = 0;
			//err = vkEnumerateDeviceExtensionProperties( device, nullptr, &count, nullptr );
			//CheckVulkanResult( err );

			//std::vector<VkExtensionProperties> available_extensions( count_extensions );
			//err = vkEnumerateDeviceExtensionProperties( device, nullptr, &count, available_extensions.data() );
			//CheckVulkanResult( err );

			//bool swapchain_support = false;
			//for ( const auto& ext : available_extensions ) {
			//    if ( ext.extensionName == VK_KHR_SWAPCHAIN_EXTENSION_NAME ) {
			//        swapchain_support = true;
			//        break;
			//    }
			//}

			if ( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU /* && swapchain_support */ )
			{
				return device;
			}
		}

		LOG_INFO( "[Vulkan] Discrete GPU is not available, first available will be selected." );

		return devices[0];
	}

	Expected<VulkanQueueFamilyIndices> Context::FindQueueFamilies( VkPhysicalDevice device, 
		VkSurfaceKHR surface )
	{
		uint32 count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( device, &count, nullptr );
		std::vector<VkQueueFamilyProperties> props( count );
		vkGetPhysicalDeviceQueueFamilyProperties( device, &count, props.data() );

		VulkanQueueFamilyIndices indices;
		int32 family_index = 0;
		for ( const auto& queue_family : props )
		{
			if ( queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				indices.Graphics = family_index;
			}

			VkBool32 present_support = false;
			VkResult err = vkGetPhysicalDeviceSurfaceSupportKHR( device, family_index, surface, 
				&present_support );
			if ( err != VK_SUCCESS )
			{
				std::string message = std::format(
					"[Vulkan] Error checking GPU surface support. "
					"vkGetPhysicalDeviceSurfaceSupportKHR returned: {}={}.",
					VK_TYPE_TO_STR( VkResult ),
					err );
				return std::unexpected( message );
			}

			if ( present_support )
			{
				indices.Present = family_index;
			}

			if ( indices.IsComplete() )
			{
				break;
			}

			++family_index;
		}
		return indices;
	}

	Expected<VkDevice> Context::CreateDevice( VulkanQueueFamilyIndices indices )
	{
		std::set<uint32> unique_families = {
			indices.Graphics.value(),
			indices.Present.value()
		};
		std::vector<VkDeviceQueueCreateInfo> queue_infos( unique_families.size() );

		float priority = 1.0f;
		for ( uint32 i = 0; i < unique_families.size(); ++i )
		{
			VkDeviceQueueCreateInfo queue_info = {};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info.queueFamilyIndex = i;
			queue_info.queueCount = 1;
			queue_info.pQueuePriorities = &priority;
			queue_infos[i] = queue_info;
		}

		VkDeviceCreateInfo device_info = {};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = static_cast< uint32 >( queue_infos.size() );
		device_info.pQueueCreateInfos = queue_infos.data();

		VkPhysicalDeviceFeatures physical_device_features = {};
		physical_device_features.samplerAnisotropy = VK_TRUE;

		device_info.pEnabledFeatures = &physical_device_features;
		device_info.enabledLayerCount = static_cast< uint32 >( ContextInfo.Layers.size() );
		device_info.ppEnabledLayerNames = ContextInfo.Layers.data();

		// the extension requires check for availability but i don't really care since i got rtx4060
		// FIX: statement in comment above is temporary there'll be a fix but for now like that
		std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		device_info.enabledExtensionCount = static_cast< uint32 >( device_extensions.size() );
		device_info.ppEnabledExtensionNames = device_extensions.data();

		VkDevice device;
		VkResult err = vkCreateDevice( PhysicalDevice, &device_info, nullptr, &device );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Device. vkCreateDevice returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		return device;
	}

	VkQueue Context::GetQueue( uint32 family_index, uint32 index )
	{
		VkQueue queue;
		vkGetDeviceQueue( Device, family_index, index, &queue );
		return queue;
	}

	Expected<VulkanSwapchain> Context::CreateSwapchain()
	{
		VkResult err;
		VkSwapchainCreateInfoKHR swapchain_info = {};
		swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.surface = Surface;

		int32 width, height;
		SDL_GetWindowSize( WindowHandle, &width, &height );

		swapchain_info.imageExtent = { static_cast<uint32>( width ), static_cast<uint32>( height ) };

		swapchain_info.minImageCount = 3;
		swapchain_info.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
		swapchain_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		swapchain_info.imageArrayLayers = 1;
		swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

		VulkanSwapchain swapchain;
		swapchain.Format = swapchain_info.imageFormat;
		swapchain.Extent = swapchain_info.imageExtent;
		err = vkCreateSwapchainKHR( Device, &swapchain_info, nullptr, &swapchain.Instance );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Swapchain. vkCreateSwapchainKHR returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		uint32 count_images = 0;
		err = vkGetSwapchainImagesKHR( Device, swapchain.Instance, &count_images, nullptr );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to receive Swapchain Images. vkGetSwapchainImagesKHR() returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		swapchain.Images.resize( count_images );

		err = vkGetSwapchainImagesKHR( Device, swapchain.Instance, &count_images, swapchain.Images.data() );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to receive swapchain images. vkGetSwapchainImageKHR() returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		return swapchain;
	}

	void Context::RecreateSwapchain()
	{
		vkDeviceWaitIdle( Device );

		Swapchain.Cleanup( Device );

		auto swapchain_result = CreateSwapchain();
		if ( !swapchain_result )
		{
			LOG_ERROR( "{}", swapchain_result.error() );
			throw std::runtime_error( "swapchain == VK_NULL_HANDLE" );
		}
		Swapchain = std::move( swapchain_result.value() );

		auto image_views_result = CreateImageViews();
		if ( !image_views_result )
		{
			LOG_ERROR( "{}", image_views_result.error() );
		}
		Swapchain.ImageViews = std::move( image_views_result.value() );

		auto texture_result = CreateDepthTexture();
		if ( !texture_result )
		{
			LOG_ERROR( "{}", texture_result.error() );
		}
		DepthTexture = std::move( texture_result.value() );

		auto framebuffers_result = CreateFramebuffers();
		if ( !framebuffers_result )
		{
			LOG_ERROR( "{}", framebuffers_result.error() );
		}
		Swapchain.Framebuffers = std::move( framebuffers_result.value() );
	}

	VkShaderModule Context::CreateShaderModule( const std::vector<char>& code )
	{
		VkShaderModuleCreateInfo module_info = {};
		module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_info.codeSize = code.size();
		ASSERT( code.size() );
		module_info.pCode = reinterpret_cast< const uint32_t* >( code.data() );

		VkShaderModule shader_module;
		VkResult err = vkCreateShaderModule( Device, &module_info, nullptr, &shader_module );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR(
				"[Vulkan] Failed to create Vulkan Shader Module. vkCreateShaderModule returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return VK_NULL_HANDLE;
		}
		return shader_module;
	}

	Expected<VulkanGraphicsPipeline> Context::CreateGraphicsPipeline( VkShaderModule vertex,
		VkShaderModule fragment )
	{
		VkResult err;
		VulkanGraphicsPipeline graphics_pipeline;

		VkDescriptorSetLayoutBinding ubo_layout_binding = {};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding sampler_layout_binding = {};
		sampler_layout_binding.binding = 1;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
			ubo_layout_binding,
			sampler_layout_binding
		};

		VkDescriptorSetLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast< uint32 >( bindings.size() );
		layout_info.pBindings = bindings.data();

		const VkAllocationCallbacks* alloc = nullptr;
		err = vkCreateDescriptorSetLayout( Device, &layout_info, alloc, &graphics_pipeline.DescriptorSetLayout );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Descriptor set layout. "
				"vkCreateDescriptorSetLayout returned {}={}",
				VK_TYPE_TO_STR( err ),
				err );
			return std::unexpected( message );
		}

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &graphics_pipeline.DescriptorSetLayout;

		err = vkCreatePipelineLayout( Device, &pipeline_layout_info, alloc, &graphics_pipeline.Layout );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Pipeline Layout."
				"vkCreatePipelineLayout returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		VkAttachmentDescription color_attachment = {};
		color_attachment.format = Swapchain.Format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto depth_format_result = FindDepthFormat();
		if ( !depth_format_result )
		{
			return std::unexpected( depth_format_result.error() );
		}
		VkFormat depth_format = std::move( depth_format_result.value() );

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_attachment = {};
		depth_attachment.format = depth_format;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		VkSubpassDependency subpass_dependency = {};
		subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependency.dstSubpass = 0;
		subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		subpass_dependency.srcAccessMask = 0;
		subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };
		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast< uint32 >( attachments.size() );
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &subpass_dependency;

		err = vkCreateRenderPass( Device, &render_pass_info, alloc, &graphics_pipeline.RenderPass );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Render Pass. vkCreateRenderPass returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		VkPipelineShaderStageCreateInfo vertex_stage_info = {};
		vertex_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertex_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertex_stage_info.module = vertex;
		vertex_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo fragment_stage_info = {};
		fragment_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragment_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragment_stage_info.module = fragment;
		fragment_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stage_infos[] = { vertex_stage_info, fragment_stage_info };

		std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.dynamicStateCount = static_cast< uint32 >( dynamic_states.size() );
		dynamic_state_info.pDynamicStates = dynamic_states.data();

		auto binding_desc = Vertex::GetBindingDescription();
		auto attribute_desc = Vertex::GetAttributeDescription();
		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast< uint32 >( attribute_desc.size() );
		vertex_input_info.pVertexBindingDescriptions = &binding_desc;
		vertex_input_info.pVertexAttributeDescriptions = attribute_desc.data();

		VkPipelineInputAssemblyStateCreateInfo assembly_info = {};
		assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkViewport viewport = {};
		viewport.width = static_cast< float >( Swapchain.Extent.width );
		viewport.height = static_cast< float >( Swapchain.Extent.height );
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.extent = Swapchain.Extent;

		VkPipelineViewportStateCreateInfo viewport_state_info = {};
		viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state_info.viewportCount = 1;
		viewport_state_info.pViewports = &viewport;
		viewport_state_info.scissorCount = 1;
		viewport_state_info.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer_info = {};
		rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer_info.lineWidth = 1.0f;
		rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		VkPipelineMultisampleStateCreateInfo multisampling_info = {};
		multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorblend_attachment = {};
		colorblend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorblend_info = {};
		colorblend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorblend_info.attachmentCount = 1;
		colorblend_info.pAttachments = &colorblend_attachment;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
		depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_info.depthTestEnable = true;
		depth_stencil_info.depthWriteEnable = true;
		depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;

		VkGraphicsPipelineCreateInfo pipeline_info = {};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stage_infos;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &assembly_info;
		pipeline_info.pViewportState = &viewport_state_info;
		pipeline_info.pRasterizationState = &rasterizer_info;
		pipeline_info.pMultisampleState = &multisampling_info;
		pipeline_info.pColorBlendState = &colorblend_info;
		pipeline_info.pDynamicState = &dynamic_state_info;
		pipeline_info.pDepthStencilState = &depth_stencil_info;
		pipeline_info.layout = graphics_pipeline.Layout;
		pipeline_info.renderPass = graphics_pipeline.RenderPass;

		const VkPipelineCache pipeline_cache = VK_NULL_HANDLE;
		const uint32          create_count = 1;
		err = vkCreateGraphicsPipelines( Device, pipeline_cache, create_count, &pipeline_info, alloc,
			&graphics_pipeline.Instance );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Graphics Pipeline. vkCreateGraphicsPipeline returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		return graphics_pipeline;
	}


	Expected<VkImageView> Context::CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspect_flags )
	{
		VkImageViewCreateInfo image_view_info = {};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = image;
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = format;
		image_view_info.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		};
		VkImageSubresourceRange subresource_range = {
			.aspectMask = aspect_flags,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		};
		image_view_info.subresourceRange = subresource_range;

		const VkAllocationCallbacks* alloc = nullptr;
		VkImageView view;
		VkResult err = vkCreateImageView( Device, &image_view_info, alloc, &view );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Image View. vkCreateImageView returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		return view;
	}

	Expected<std::vector<VkImageView>> Context::CreateImageViews()
	{
		std::vector<VkImageView> image_views( Swapchain.Images.size() );

		for ( size_t i = 0; i < Swapchain.Images.size(); ++i )
		{
			auto view_result = CreateImageView( Swapchain.Images[i], Swapchain.Format, 
				VK_IMAGE_ASPECT_COLOR_BIT );
			if ( !view_result )
			{
				return std::unexpected( view_result.error() );
			}
			image_views[i] = std::move( view_result.value() );
		}
		return image_views;
	}

	Expected<std::vector<VkFramebuffer>> Context::CreateFramebuffers()
	{
		std::vector<VkFramebuffer> framebuffers( Swapchain.ImageViews.size() );
		for ( size_t i = 0; i < framebuffers.size(); i++ )
		{
			std::array<VkImageView, 2> attachments = {
				Swapchain.ImageViews[i],
				DepthTexture.View
			};

			VkFramebufferCreateInfo framebuffer_info = {};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = GraphicsPipeline.RenderPass;
			framebuffer_info.attachmentCount = static_cast< uint32 >( attachments.size() );
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = Swapchain.Extent.width;
			framebuffer_info.height = Swapchain.Extent.height;
			framebuffer_info.layers = 1;

			VkResult err = vkCreateFramebuffer( Device, &framebuffer_info, nullptr, &framebuffers[i] );
			if ( err != VK_SUCCESS )
			{
				std::string message = std::format(
					"[Vulkan] Failed to create framebuffer. vkCreateFramebuffer returned: {}={}",
					VK_TYPE_TO_STR( VkResult ),
					err );
				return std::unexpected( message );
			}
		}
		return framebuffers;
	}

	Expected<VkCommandPool> Context::CreateCommandPool( VulkanQueueFamilyIndices indices )
	{
		VkCommandPoolCreateInfo cmdpool_info = {};
		cmdpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmdpool_info.queueFamilyIndex = indices.Graphics.value();

		VkCommandPool command_pool;
		VkResult err = vkCreateCommandPool( Device, &cmdpool_info, nullptr, &command_pool );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Command Pool. vkCreateCommandPool returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		return command_pool;
	}

	std::vector<VkCommandBuffer> Context::CreateCommandBuffers()
	{
		std::vector<VkCommandBuffer> command_buffers( MAX_FRAMES_IN_FLIGHT );

		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = CommandPool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = static_cast<uint32>( command_buffers.size() );

		VkResult err = vkAllocateCommandBuffers( Device, &alloc_info, command_buffers.data() );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR(
				"[Vulkan] Error allocating Vulkan Command Buffers. vkAllocateCommandBuffers returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err
			);
			return {};
		}
		return command_buffers;
	}

	Expected<std::vector<VulkanSyncObjects>> Context::CreateSyncObjects()
	{
		VkResult err;
		std::vector<VulkanSyncObjects> sync_objs( MAX_FRAMES_IN_FLIGHT );

		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info = {};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		const VkAllocationCallbacks* alloc = nullptr;

		for ( VulkanSyncObjects& obj : sync_objs )
		{
			err = vkCreateSemaphore( Device, &semaphore_info, alloc, &obj.ImageAvailableSemaphores );
			if ( err != VK_SUCCESS )
			{
				std::string message = std::format(
					"[Vulkan] Failed to create Image Available semaphore object. "
					"vkCreateSemaphore returned {}={}",
					VK_TYPE_TO_STR( VkResult ),
					err );
				return std::unexpected( message );
			}

			err = vkCreateSemaphore( Device, &semaphore_info, alloc, &obj.RenderFinishedSemaphores );
			if ( err != VK_SUCCESS )
			{
				std::string message = std::format(
					"[Vulkan] Failed to create Render Finished semaphore object. "
					"vkCreateSemaphore returned {}={}",
					VK_TYPE_TO_STR( VkResult ),
					err );
				return std::unexpected( message );
			}

			err = vkCreateFence( Device, &fence_info, alloc, &obj.InFlightFences );
			if ( err != VK_SUCCESS )
			{
				std::string message = std::format(
					"[Vulkan] Failed to create Fences. vkCreateFence returned {}={}",
					VK_TYPE_TO_STR( VkResult ),
					err );
				return std::unexpected( message );
			}
		}
		return sync_objs;
	}

	uint32 Context::FindMemoryType( uint32 type_filter, VkMemoryPropertyFlags prop_flags )
	{
		VkPhysicalDeviceMemoryProperties memory_props = {};
		vkGetPhysicalDeviceMemoryProperties( PhysicalDevice, &memory_props );

		for ( uint32 i = 0; memory_props.memoryTypeCount; ++i )
		{
			if ( ( type_filter & ( 1 << i ) ) &&
				( memory_props.memoryTypes[i].propertyFlags & prop_flags ) == prop_flags )
			{
				return i;
			}
		}

		throw std::runtime_error( "failed to find suitable memory type" );
	}

	Expected<VulkanBuffer> Context::CreateBuffer( VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags props )
	{
		VkResult err;
		VulkanBuffer buffer;

		VkBufferCreateInfo buffer_info = {};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		const VkAllocationCallbacks* alloc = nullptr;
		err = vkCreateBuffer( Device, &buffer_info, alloc, &buffer.Instance );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan Buffer. vkCreate buffer returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		VkMemoryRequirements memory_requirements = {};
		vkGetBufferMemoryRequirements( Device, buffer.Instance, &memory_requirements );

		VkMemoryAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_info.allocationSize = memory_requirements.size;
		allocate_info.memoryTypeIndex = FindMemoryType( memory_requirements.memoryTypeBits, props );

		err = vkAllocateMemory( Device, &allocate_info, alloc, &buffer.Memory );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to allocate memory. vkAllocateMemory returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		const VkDeviceSize memory_offset = 0;
		err = vkBindBufferMemory( Device, buffer.Instance, buffer.Memory, memory_offset );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to bind buffer memory. vkBindBufferMemory returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err
			);
			return std::unexpected( message );
		}
		return buffer;
	}

	Expected<VulkanBuffer> Context::CreateVertexBuffer()
	{
		const VkDeviceSize    buffer_size = sizeof( VERTICES[0] ) * VERTICES.size();
		VkBufferUsageFlags    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		auto staging_buffer_result = CreateBuffer( buffer_size, usage, props );
		if ( !staging_buffer_result )
		{
			return std::unexpected( staging_buffer_result.error() );
		}
		VulkanBuffer staging_buffer = *staging_buffer_result;

		const VkDeviceSize     offset = 0;
		const VkMemoryMapFlags flags = 0;
		void* data = nullptr;
		VkResult err = vkMapMemory( Device, staging_buffer.Memory, offset, buffer_size, flags, &data );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to map memory. vkMapMemory returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		memcpy( data, VERTICES.data(), static_cast< size_t >( buffer_size ) );
		vkUnmapMemory( Device, staging_buffer.Memory );

		usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		auto vertex_buffer_result = CreateBuffer( buffer_size, usage, props );
		if ( !vertex_buffer_result )
		{
			return std::unexpected( vertex_buffer_result.error() );
		}
		VulkanBuffer vertex_buffer = *vertex_buffer_result;

		CopyBuffer( staging_buffer.Instance, vertex_buffer.Instance, buffer_size );

		const VkAllocationCallbacks* alloc = nullptr;
		vkDestroyBuffer( Device, staging_buffer.Instance, alloc );
		vkFreeMemory( Device, staging_buffer.Memory, alloc );

		return vertex_buffer;
	}

	Expected<VulkanBuffer> Context::CreateIndexBuffer()
	{
		VkDeviceSize buffer_size = sizeof( INDICES[0] ) * INDICES.size();

		VkBufferUsageFlags    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		auto staging_buffer_result = CreateBuffer( buffer_size, usage, props );
		if ( !staging_buffer_result )
		{
			return std::unexpected( staging_buffer_result.error() );
		}
		VulkanBuffer staging_buffer = *staging_buffer_result;

		const VkDeviceSize     offset = 0;
		const VkMemoryMapFlags flags = 0;
		void* data;
		VkResult err = vkMapMemory( Device, staging_buffer.Memory, offset, buffer_size, flags, &data );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to map memory. vkMapMemory returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		memcpy( data, INDICES.data(), static_cast< size_t >( buffer_size ) );
		vkUnmapMemory( Device, staging_buffer.Memory );

		usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		auto index_buffer_result = CreateBuffer( buffer_size, usage, props );
		if ( !index_buffer_result )
		{
			return std::unexpected( index_buffer_result.error() );
		}
		VulkanBuffer index_buffer = std::move( index_buffer_result.value() );

		CopyBuffer( staging_buffer.Instance, index_buffer.Instance, buffer_size );

		const VkAllocationCallbacks* alloc = nullptr;
		vkDestroyBuffer( Device, staging_buffer.Instance, alloc );
		vkFreeMemory( Device, staging_buffer.Memory, alloc );

		return index_buffer;
	}

	Expected<std::vector<VulkanBuffer>> Context::CreateUniformBuffers()
	{
		std::vector<VulkanBuffer> uniform_buffers( MAX_FRAMES_IN_FLIGHT );

		VkDeviceSize buffer_size = sizeof( UniformBufferObject );
		for ( VulkanBuffer& uniform_buffer : uniform_buffers )
		{
			VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			auto uniform_buffer_result = CreateBuffer( buffer_size, buffer_usage_flags, props );
			if ( !uniform_buffer_result )
			{
				return std::unexpected( uniform_buffer_result.error() );
			}
			uniform_buffer = std::move( uniform_buffer_result.value() );

			VkDeviceSize	 offset = 0;
			VkMemoryMapFlags memory_map_flags = 0;
			VkResult err = vkMapMemory( Device, uniform_buffer.Memory, offset, buffer_size, memory_map_flags,
				&uniform_buffer.Mapped );
			if ( err != VK_SUCCESS )
			{
				std::string message = std::format(
					"[Vulkan] Failed to map memory. vkMapMemory returned {}={}.",
					VK_TYPE_TO_STR( VkResult ),
					err );
				return std::unexpected( message );
			}
		}

		return uniform_buffers;
	}

	void Context::CopyBuffer( VkBuffer source, VkBuffer destination, VkDeviceSize size )
	{
		auto command_buffer_result = BeginSingleTimeCommands();
		if ( !command_buffer_result )
		{
			LOG_ERROR( "{}", command_buffer_result.error() );
			throw std::runtime_error( "BeginSingleTimeCommands failed" );
		}
		VkCommandBuffer command_buffer = std::move( command_buffer_result.value() );

		VkBufferCopy buffer_copy = {};
		buffer_copy.size = size;

		const uint32 region_count = 1;
		vkCmdCopyBuffer( command_buffer, source, destination, region_count, &buffer_copy );

		EndSingleTimeCommands( command_buffer );
	}

	void Context::UpdateUniformBuffer( uint32 current_image )
	{
		namespace chrono = std::chrono;
		static auto start_time = chrono::high_resolution_clock::now();

		auto current_time = chrono::high_resolution_clock::now();
		float time = chrono::duration<float, chrono::seconds::period>( current_time - start_time ).count();

		UniformBufferObject ubo = {};
		ubo.Model = glm::rotate(
			glm::mat4( 1.0f ),
			time * glm::radians( 90.0f ),
			glm::vec3( 0.0f, 0.0f, 1.0f ) );

		ubo.View = glm::lookAt(
			glm::vec3( 1.0f ),
			glm::vec3( 0.0f ),
			glm::vec3( 0.0f, 0.0f, 1.0f ) );

		ubo.Projection = glm::perspective(
			glm::radians( 45.0f ),
			Swapchain.Extent.width / static_cast< float >( Swapchain.Extent.height ),
			0.1f,
			10.0f );

		ubo.Projection[1][1] *= -1;

		memcpy( UniformBuffers[current_image].Mapped, &ubo, sizeof( ubo ) );
	}

	Expected<VulkanDescriptorGroup> Context::CreateDescriptorGroup()
	{
		VkResult err;
		VulkanDescriptorGroup descriptor_group = {};

		std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
		pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[0].descriptorCount = static_cast< uint32 >( MAX_FRAMES_IN_FLIGHT );
		pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[1].descriptorCount = static_cast< uint32 >( MAX_FRAMES_IN_FLIGHT );

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.poolSizeCount = 2;
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.maxSets = static_cast< uint32 >( MAX_FRAMES_IN_FLIGHT );

		const VkAllocationCallbacks* alloc = nullptr;
		err = vkCreateDescriptorPool( Device, &pool_info, alloc, &descriptor_group.Pool );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Descriptor Pool. vkCreateDescriptorPool returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT,
			GraphicsPipeline.DescriptorSetLayout );
		VkDescriptorSetAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocate_info.descriptorPool = descriptor_group.Pool;
		allocate_info.descriptorSetCount = static_cast< uint32 >( MAX_FRAMES_IN_FLIGHT );
		allocate_info.pSetLayouts = layouts.data();

		descriptor_group.Sets.resize( MAX_FRAMES_IN_FLIGHT );
		err = vkAllocateDescriptorSets( Device, &allocate_info, descriptor_group.Sets.data() );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to allocate Descriptor Sets. vkAllocateDescriptorSets returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
		{
			VkDescriptorBufferInfo buffer_info = {};
			buffer_info.buffer = UniformBuffers[i].Instance;
			buffer_info.offset = 0;
			buffer_info.range = sizeof( UniformBufferObject );

			VkDescriptorImageInfo image_info = {};
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info.imageView = Texture.View;
			image_info.sampler = Texture.Sampler;

			std::array<VkWriteDescriptorSet, 2> descriptor_writes = {};
			descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[0].dstSet = descriptor_group.Sets[i];
			descriptor_writes[0].dstBinding = 0;
			descriptor_writes[0].dstArrayElement = 0;
			descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writes[0].descriptorCount = 1;
			descriptor_writes[0].pBufferInfo = &buffer_info;

			descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[1].dstSet = descriptor_group.Sets[i];
			descriptor_writes[1].dstBinding = 1;
			descriptor_writes[1].dstArrayElement = 0;
			descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_writes[1].descriptorCount = 1;
			descriptor_writes[1].pImageInfo = &image_info;

			const uint32 descriptor_write_count = static_cast< uint32 >( descriptor_writes.size() );
			const uint32 descriptor_copy_count = 0;
			const VkCopyDescriptorSet* descriptor_copies = nullptr;
			vkUpdateDescriptorSets( Device, descriptor_write_count, descriptor_writes.data(),
				descriptor_copy_count, descriptor_copies );
		}
		return descriptor_group;
	}

	Expected<VulkanTexture> Context::CreateTexture()
	{
		VkResult err;

		auto assets_path = Application::ExecutablePath()
			.parent_path().parent_path().parent_path().parent_path().parent_path() / "Assets" / "tyler.jpg";
		std::string assets_path_string = assets_path.string();

		int32 width;
		int32 height;
		int32 channels;
		stbi_uc* pixels = stbi_load( assets_path_string.c_str(), &width, &height, &channels, STBI_rgb_alpha );
		if ( !pixels )
		{
			std::string message = std::format( "[Vulkan] Failed to load {}", assets_path_string );
			return std::unexpected( message );
		}

		VkDeviceSize image_size = width * height * 4;

		VkBufferUsageFlags    flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		auto staging_buffer_result = CreateBuffer( image_size, flags, props );
		if ( !staging_buffer_result )
		{
			return std::unexpected( staging_buffer_result.error() );
		}
		VulkanBuffer staging_buffer = std::move( staging_buffer_result.value() );

		const VkDeviceSize offset = 0;
		const VkMemoryMapFlags memory_flags = 0;
		void* data = nullptr;
		err = vkMapMemory( Device, staging_buffer.Memory, offset, image_size, memory_flags, &data );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to map memory. vkMapMemory returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		memcpy( data, pixels, static_cast< size_t >( image_size ) );
		vkUnmapMemory( Device, staging_buffer.Memory );

		stbi_image_free( pixels );

		flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		auto texture_image_result = CreateTextureImage( width, height, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL, flags, props );
		if ( !texture_image_result )
		{
			return std::unexpected( texture_image_result.error() );
		}
		VulkanTexture texture = std::move( texture_image_result.value() );

		VkImageLayout old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		TransitionImageLayout( texture, VK_FORMAT_R8G8B8A8_SRGB, old_layout, new_layout );
		CopyBufferToImage(
			staging_buffer.Instance,
			texture.Image,
			static_cast< uint32 >( width ),
			static_cast< uint32 >( height ) );

		old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		TransitionImageLayout( texture, VK_FORMAT_R8G8B8A8_SRGB, old_layout, new_layout );

		const VkAllocationCallbacks* alloc = nullptr;
		vkDestroyBuffer( Device, staging_buffer.Instance, alloc );
		vkFreeMemory( Device, staging_buffer.Memory, alloc );

		auto view_result = CreateImageView( texture.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT );
		if ( !view_result )
		{
			return std::unexpected( view_result.error() );
		}
		texture.View = std::move( view_result.value() );

		VkSamplerCreateInfo sampler_info = {};
		sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;
		sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler_info.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties physical_props = {};
		vkGetPhysicalDeviceProperties( PhysicalDevice, &physical_props );

		sampler_info.maxAnisotropy = physical_props.limits.maxSamplerAnisotropy;
		sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler_info.unnormalizedCoordinates = VK_FALSE;
		sampler_info.compareEnable = VK_FALSE;
		sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler_info.mipLodBias = 0.0f;
		sampler_info.minLod = 0.0f;
		sampler_info.maxLod = 0.0f;

		err = vkCreateSampler( Device, &sampler_info, alloc, &texture.Sampler );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan texture sampler. vkCreateSampler returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		return texture;
	}

	Expected<VulkanTexture> Context::CreateTextureImage( int32 width, int32 height,
		VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_props )
	{
		VkResult err;
		VulkanTexture texture;
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.extent.width = static_cast< uint32 >( width );
		image_info.extent.height = static_cast< uint32 >( height );
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = tiling;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage = usage;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;

		const VkAllocationCallbacks* alloc = nullptr;
		err = vkCreateImage( Device, &image_info, alloc, &texture.Image );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to create Vulkan texture image. vkCreateImage returned: {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		VkMemoryRequirements memory_requirements = {};
		vkGetImageMemoryRequirements( Device, texture.Image, &memory_requirements );

		VkMemoryAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_info.allocationSize = memory_requirements.size;
		allocate_info.memoryTypeIndex = FindMemoryType( memory_requirements.memoryTypeBits, memory_props );

		err = vkAllocateMemory( Device, &allocate_info, alloc, &texture.Memory );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to allocate memory for Vulkan texture. vkAllocateMemory returned: {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		const VkDeviceSize memory_offset = 0;
		err = vkBindImageMemory( Device, texture.Image, texture.Memory, memory_offset );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to bind Vulkan texture memory. vkBindImageMemory returned: {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		return texture;
	}

	Expected<VulkanTexture> Context::CreateDepthTexture()
	{
		VkResult err;

		auto format_result = FindDepthFormat();
		if ( !format_result )
		{
			return std::unexpected( format_result.error() );
		}
		VkFormat format = format_result.value();

		const int32 width = Swapchain.Extent.width;
		const int32 height = Swapchain.Extent.height;
		auto texture_image_result = CreateTextureImage( width, height, format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
		if ( !texture_image_result )
		{
			return std::unexpected( texture_image_result.error() );
		}
		VulkanTexture texture = std::move( texture_image_result.value() );

		auto view_result = CreateImageView( texture.Image, format, VK_IMAGE_ASPECT_DEPTH_BIT );
		if ( !view_result )
		{
			return std::unexpected( view_result.error() );
		}
		texture.View = std::move( view_result.value() );

		TransitionImageLayout( texture, format, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );

		return texture;
	}

	void Context::RecordCommandBuffer( uint32 image_index )
	{
		VkResult err;
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		err = vkBeginCommandBuffer( CommandBuffers[CurrentFrame], &begin_info );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR(
				"[Vulkan] Error beginning recording Vulkan Command Buffer. "
				"vkBeginCommandBuffer returned: {} = {}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			throw std::runtime_error( "failed to begin recording command buffer" );
		}

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = GraphicsPipeline.RenderPass;
		render_pass_info.framebuffer = Swapchain.Framebuffers[image_index];
		render_pass_info.renderArea.offset = { 0,0 };
		render_pass_info.renderArea.extent = Swapchain.Extent;

		std::array<VkClearValue, 2> colors = {};
		colors[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		colors[1].depthStencil = { 1.0f, 0 };

		render_pass_info.clearValueCount = static_cast< uint32 >( colors.size() );
		render_pass_info.pClearValues = colors.data();

		vkCmdBeginRenderPass( CommandBuffers[CurrentFrame], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE );
		vkCmdBindPipeline( CommandBuffers[CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsPipeline.Instance );

		VkBuffer vertex_buffers[] = { VertexBuffer.Instance };
		VkDeviceSize offsets[] = { 0 };

		const uint32 first_binding = 0;
		const uint32 binding_count = 1;
		vkCmdBindVertexBuffers( CommandBuffers[CurrentFrame], first_binding, binding_count, vertex_buffers,
			offsets );

		const VkDeviceSize offset = 0;
		vkCmdBindIndexBuffer( CommandBuffers[CurrentFrame], IndexBuffer.Instance, offset, VK_INDEX_TYPE_UINT16 );

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast< float >( Swapchain.Extent.width );
		viewport.height = static_cast< float >( Swapchain.Extent.height );
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport( CommandBuffers[CurrentFrame], 0, 1, &viewport );

		VkRect2D scissor = {};
		scissor.offset = { 0,0 };
		scissor.extent = Swapchain.Extent;
		vkCmdSetScissor( CommandBuffers[CurrentFrame], 0, 1, &scissor );

		const uint32  first_set = 0;
		const uint32  descriptor_set_count = 1;
		const uint32  dynamic_offset_count = 0;
		const uint32* dynamic_offsets = nullptr;
		vkCmdBindDescriptorSets(
			CommandBuffers[CurrentFrame],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GraphicsPipeline.Layout,
			first_set,
			descriptor_set_count,
			&DescriptorGroup.Sets[CurrentFrame],
			dynamic_offset_count,
			dynamic_offsets
		);

		const uint32 instance_count = 1;
		const uint32 first_index = 0;
		const int32  vertex_offset = 0;
		const uint32 first_instance = 0;
		vkCmdDrawIndexed(
			CommandBuffers[CurrentFrame],
			static_cast< uint32 >( INDICES.size() ),
			instance_count,
			first_index,
			vertex_offset,
			first_instance
		);

		vkCmdEndRenderPass( CommandBuffers[CurrentFrame] );

		err = vkEndCommandBuffer( CommandBuffers[CurrentFrame] );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR(
				"[Vulkan] Error ending recording Vulkan Command Buffer. vkEndCommandBuffer returned: {}={}",
				VK_TYPE_TO_STR( VkResult ),
				err );
			throw std::runtime_error( "failed to end recording command buffer" );
		}
	}

	Expected<VkCommandBuffer> Context::BeginSingleTimeCommands()
	{
		VkResult err;
		VkCommandBufferAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandPool = CommandPool;
		allocate_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		err = vkAllocateCommandBuffers( Device, &allocate_info, &command_buffer );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to allocate command buffers. vkAllocateCommandBuffers returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		err = vkBeginCommandBuffer( command_buffer, &begin_info );
		if ( err != VK_SUCCESS )
		{
			std::string message = std::format(
				"[Vulkan] Failed to begin command buffer. vkBeginCommandBuffer returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			return std::unexpected( message );
		}
		return command_buffer;
	}

	void Context::EndSingleTimeCommands( VkCommandBuffer command_buffer )
	{
		VkResult err;
		err = vkEndCommandBuffer( command_buffer );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR(
				"[Vulkan] Failed to end command buffer. vkEndCommandBuffer returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			throw std::runtime_error( "vkEndCommandBuffer failed" );
		}

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		const VkFence fence = VK_NULL_HANDLE;
		err = vkQueueSubmit( GraphicsQueue, 1, &submit_info, fence );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR(
				"[Vulkan] Failed to submit queue. vkQueueSubmit returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			throw std::runtime_error( "vkQueueSubmit failed" );
		}

		err = vkQueueWaitIdle( GraphicsQueue );
		if ( err != VK_SUCCESS )
		{
			LOG_ERROR(
				"[Vulkan] Failed to wait for queue idle. vkQueueWaitIdle returned {}={}.",
				VK_TYPE_TO_STR( VkResult ),
				err );
			throw std::runtime_error( "vkQeueuWaitIdle failed" );
		}

		const uint32 command_buffer_count = 1;
		vkFreeCommandBuffers( Device, CommandPool, command_buffer_count, &command_buffer );
	}

	void Context::TransitionImageLayout( const VulkanTexture& texture, VkFormat format,
		VkImageLayout old_layout, VkImageLayout new_layout )
	{
		auto command_buffer_result = BeginSingleTimeCommands();
		if ( !command_buffer_result )
		{
			LOG_ERROR( "{}", command_buffer_result.error() );
			throw std::runtime_error( "BeginSingleTimeCommands failed" );
		}
		VkCommandBuffer command_buffer = std::move( command_buffer_result.value() );

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = old_layout;
		barrier.newLayout = new_layout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texture.Image;

		VkImageSubresourceRange subresource_range = {};
		subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range.baseMipLevel = 0;
		subresource_range.levelCount = 1;
		subresource_range.baseArrayLayer = 0;
		subresource_range.layerCount = 1;

		barrier.subresourceRange = subresource_range;
		barrier.srcAccessMask = 0; // TODO
		barrier.dstAccessMask = 0; // TODO

		VkPipelineStageFlags source_stage;
		VkPipelineStageFlags destination_stage;

		if ( new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if ( format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT )
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if ( old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
			new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			throw std::runtime_error( "unsupported layout transition" );
		}

		vkCmdPipelineBarrier( command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1,
			&barrier );
		EndSingleTimeCommands( command_buffer );
	}

	void Context::CopyBufferToImage( VkBuffer buffer, VkImage image, uint32 width, uint32 height )
	{
		auto command_buffer_result = BeginSingleTimeCommands();
		if ( !command_buffer_result )
		{
			LOG_ERROR( "{}", command_buffer_result.error() );
			throw std::runtime_error( "BeginSingleTimeCommands failed" );
		}
		VkCommandBuffer command_buffer = std::move( command_buffer_result.value() );

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		VkImageSubresourceLayers image_subresource = {};
		image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_subresource.mipLevel = 0;
		image_subresource.baseArrayLayer = 0;
		image_subresource.layerCount = 1;

		region.imageSubresource = image_subresource;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			command_buffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);

		EndSingleTimeCommands( command_buffer );
	}

	Expected<VkFormat> Context::FindSupportedFormat( std::span<const VkFormat> candidates,
		VkImageTiling tiling, VkFormatFeatureFlags features )
	{
		for ( VkFormat format : candidates )
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties( PhysicalDevice, format, &props );

			if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features )
			{
				return format;
			}
			else if ( tiling == VK_IMAGE_TILING_OPTIMAL &&
				( props.optimalTilingFeatures & features ) == features )
			{
				return format;
			}
		}

		return std::unexpected( "[Vulkan] Failed to find supported format." );
	}

	Expected<VkFormat> Context::FindDepthFormat()
	{
		std::vector formats = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};
		return FindSupportedFormat( formats, VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
	}

} // namespace VulkanRHI 

