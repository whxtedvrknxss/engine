#include "VulkanContext.h"

#include <set>
#include <chrono>
#include <stdexcept>

#include <SDL3/SDL_vulkan.h>>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Engine/Core/Common.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Assert.h"
#include "Shader.h"
#include "VulkanMath.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <Engine/Core/Application.h>

#define VK_TYPE_TO_STR(type) #type

VulkanContext::VulkanContext( VulkanContextCreateInfo& context_info, SDL_Window* window_handle )
{
    ContextInfo = std::move( context_info );
    WindowHandle = window_handle;

    Instance = VK_NULL_HANDLE;
    Surface = VK_NULL_HANDLE;
    PhysicalDevice = VK_NULL_HANDLE;
    Device = VK_NULL_HANDLE;
    GraphicsQueue = VK_NULL_HANDLE;
    PresentQueue = VK_NULL_HANDLE;
    Swapchain = { VK_NULL_HANDLE};
    GraphicsPipeline = {};
    CommandPool = VK_NULL_HANDLE;
    SyncObjects = {};
    VertexBuffer = {};
    IndexBuffer = {};
    StagingBuffer = {};
}

VulkanContext::~VulkanContext()
{
    Cleanup();
}

void VulkanContext::Init()
{
    Instance = CreateInstance();
    if ( Instance == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "Instance == VK_NULL_HANDLE" );
    }

    Surface = CreateSurface();
    if ( Surface == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "Surface == VK_NULL_HANDLE" );
    }

    PhysicalDevice = SelectPhysicalDevice();
    if ( PhysicalDevice == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "PhysicalDevice == VK_NULL_HANDLE" );
    }

    QueueFamilyIndices indices = FindQueueFamilies( PhysicalDevice, Surface );

    Device = CreateDevice( indices );
    if ( Device == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "Device == VK_NULL_HANDLE" );
    }

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

    Swapchain = CreateSwapchain();
    if ( Swapchain.Instance == VK_NULL_HANDLE || Swapchain.Images.size() == 0 )
    {
        throw std::runtime_error( "Swapchain == VK_NULL_HANDLE || SwapchainImages.size == 0" );
    }

    ImageViews = CreateImageViews();
    if ( ImageViews.size() == 0 )
    {
        throw std::runtime_error( "ImageViews.size == 0" );
    }

    auto shaders_path = std::filesystem::current_path().parent_path() / "Engine" / "Shaders";
    auto vertex_shader = GetShaderSource( shaders_path / "triangle.vert.spv" );
    auto fragment_shader = GetShaderSource( shaders_path / "triangle.frag.spv" );

    VkShaderModule vertex = CreateShaderModule( vertex_shader );
    VkShaderModule fragment = CreateShaderModule( fragment_shader );
    if ( vertex == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "vertex == VK_NULL_HANDLE" );
    }

    if ( fragment == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "fragment == VK_NULL_HANDLE" );
    }

    GraphicsPipeline = CreateGraphicsPipeline( vertex, fragment );
    if ( GraphicsPipeline.Instance == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "GraphicsPipeline == VK_NULL_HANDLE" );
    }

    vkDestroyShaderModule( Device, vertex, nullptr );
    vkDestroyShaderModule( Device, fragment, nullptr );

    Framebuffers = CreateFramebuffers();
    for ( int32 i = 0; i < Framebuffers.size(); ++i )
    {
        if ( Framebuffers[i] == VK_NULL_HANDLE )
        {
            throw std::runtime_error( "Framebuffer == VK_NULL_HANDLE" );
        }
    }

    CommandPool = CreateCommandPool( indices );
    if ( CommandPool == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "CommandPool == VK_NULL_HANDLE" );
    }

    CommandBuffers = CreateCommandBuffers();
    for ( int32 i = 0; i < CommandBuffers.size(); ++i )
    {
        if ( CommandBuffers[i] == VK_NULL_HANDLE )
        {
            throw std::runtime_error( "CommandBuffer == VK_NULL_HANDLE" );
        }
    }

    SyncObjects = CreateSyncObjects();
    for ( int32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        bool sync_objs_invalid = SyncObjects.ImageAvailableSemaphores[i] == VK_NULL_HANDLE ||
                                 SyncObjects.InFlightFences[i] == VK_NULL_HANDLE ||
                                 SyncObjects.RenderFinishedSemaphores[i] == VK_NULL_HANDLE;
        if ( sync_objs_invalid )
        {
            throw std::runtime_error( "sync objects are invalid" );
        }
    }

	Texture = CreateTexture();
    if ( Texture.Image == VK_NULL_HANDLE || Texture.Memory == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "texture image == VK_NULL_HANDLE" );
    }

    VertexBuffer = CreateVertexBuffer();
    if ( VertexBuffer.Instance == VK_NULL_HANDLE || VertexBuffer.Memory == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "VertexBuffer == VK_NULL_HANDLE || VertexBufferMemory == VK_NULL_HANDLE" );
    }

    IndexBuffer = CreateIndexBuffer();
    if ( IndexBuffer.Instance == VK_NULL_HANDLE || IndexBuffer.Memory == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "IndexBuffer == VK_NULL_HANDLE || IndexBufferMemory == VK_NULL_HANDLE" );
    }

    UniformBuffers = CreateUniformBuffers();
    for ( int32 i = 0; i < UniformBuffers.size(); ++i )
    {
       
    }

    DescriptorGroup = CreateDescriptorGroup();
    if ( DescriptorGroup.Pool == VK_NULL_HANDLE )
    {
		throw std::runtime_error( "DescriptorPool == VK_NULL_HANDLE" );
    }
}

void VulkanContext::Cleanup()
{
    const VkAllocationCallbacks* alloc = nullptr;

    vkDeviceWaitIdle( Device );

    CleanupSwapchain();

    vkDestroySampler( Device, Texture.Sampler, alloc );
    vkDestroyImageView( Device, Texture.View, alloc );
    vkDestroyImage( Device, Texture.Image, alloc );
    vkFreeMemory( Device, Texture.Memory, alloc );

    vkDestroyDescriptorPool( Device, DescriptorGroup.Pool, alloc );

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        vkDestroyBuffer( Device, UniformBuffers[i].Instance, alloc );
        vkFreeMemory( Device, UniformBuffers[i].Memory, alloc );
    }

    vkDestroyBuffer( Device, IndexBuffer.Instance, alloc );
    vkFreeMemory( Device, IndexBuffer.Memory, alloc );

    vkDestroyBuffer( Device, VertexBuffer.Instance, alloc );
    vkFreeMemory( Device, VertexBuffer.Memory, alloc );

    vkDestroyDescriptorSetLayout( Device, GraphicsPipeline.DescriptorSetLayout, alloc );
    vkDestroyPipeline( Device, GraphicsPipeline.Instance, alloc );
    vkDestroyPipelineLayout( Device, GraphicsPipeline.Layout, alloc );
    vkDestroyRenderPass( Device, GraphicsPipeline.RenderPass, alloc );

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        vkDestroySemaphore( Device, SyncObjects.ImageAvailableSemaphores[i], alloc );
        vkDestroySemaphore( Device, SyncObjects.RenderFinishedSemaphores[i], alloc );
        vkDestroyFence( Device, SyncObjects.InFlightFences[i], alloc );
    }

    vkDestroyCommandPool( Device, CommandPool, alloc );

    vkDestroyDevice( Device, alloc );
    vkDestroySurfaceKHR( Instance, Surface, alloc );
    vkDestroyInstance( Instance, alloc );
}

void VulkanContext::DrawFrame()
{
    VkResult err;

    const uint32   FENCE_COUNT = 1;
    const VkBool32 WAIT_ALL = VK_TRUE;
    const uint64   TIMEOUT = UINT64_MAX;
    err = vkWaitForFences( Device, FENCE_COUNT, &SyncObjects.InFlightFences[CurrentFrame], WAIT_ALL, TIMEOUT );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( "[Vulkan] Error from vkWaitForFences: {}", err );
    }

    uint32  image_index;
    VkFence FENCE = VK_NULL_HANDLE;
    err = vkAcquireNextImageKHR( Device, Swapchain.Instance, TIMEOUT,
        SyncObjects.ImageAvailableSemaphores[CurrentFrame], FENCE, &image_index );
    if ( err == VK_ERROR_OUT_OF_DATE_KHR )
    {
        RecreateSwapchain();
        return;
    }
    else if ( err != VK_SUCCESS && err != VK_SUBOPTIMAL_KHR )
    {
        LOG_ERROR( "[Vulkan] Error from vkAcquireNextImageKHR: %d", err );
    }

    UpdateUniformBuffer(CurrentFrame);

    err = vkResetFences( Device, FENCE_COUNT, &SyncObjects.InFlightFences[CurrentFrame] );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( "[Vulkan] Error from vkResetFences: %d", err );
    }

    err = vkResetCommandBuffer( CommandBuffers[CurrentFrame], 0 );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( "[Vulkan] Error from vkResetCommandBuffer: %d", err );
    }
    RecordCommandBuffer( image_index );

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = { SyncObjects.ImageAvailableSemaphores[CurrentFrame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &CommandBuffers[CurrentFrame];

    VkSemaphore signal_semaphores[] = { SyncObjects.RenderFinishedSemaphores[CurrentFrame] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    const uint32 SUBMIT_COUNT = 1;
    err = vkQueueSubmit( GraphicsQueue, SUBMIT_COUNT, &submit_info, SyncObjects.InFlightFences[CurrentFrame] );
    if ( err != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to submit draw command buffer!" );
    }

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = { Swapchain.Instance };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;

    err = vkQueuePresentKHR( PresentQueue, &present_info );
    if ( err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR )
    {
        RecreateSwapchain();
    }
    else if ( err != VK_SUCCESS )
    {
        // LOG_ERROR
    }

    CurrentFrame = ( CurrentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
}

bool VulkanContext::IsExtensionAvailable( const std::vector<VkExtensionProperties>& props, const char* extension )
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

bool VulkanContext::IsLayerAvailable( const std::vector<VkLayerProperties>& props, const char* layer )
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

VkInstance VulkanContext::CreateInstance()
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

    uint32 count_ext = 0;
    err = vkEnumerateInstanceExtensionProperties( nullptr, &count_ext, nullptr );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to enumerate Vulkan Instance extension properties. "
            "vkEnumerateInstanceExtensionProperties returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err
        );
        return VK_NULL_HANDLE;
    }

    std::vector<VkExtensionProperties> available_ext( count_ext );
    err = vkEnumerateInstanceExtensionProperties( nullptr, &count_ext, available_ext.data() );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to enumerate Vulkan Instance extension properties. "
            "vkEnumerateInstanceExtensionProperties returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return VK_NULL_HANDLE;
    }

    for ( const auto& ext : ContextInfo.Extensions )
    {
        if ( !IsExtensionAvailable( available_ext, ext ) )
        {
            LOG_ERROR( "[Vulkan] Required extension is not available. Extension name: %s", ext );
            return VK_NULL_HANDLE;
        }
    }

    instance_info.enabledExtensionCount = static_cast< uint32 >( ContextInfo.Extensions.size() );
    instance_info.ppEnabledExtensionNames = ContextInfo.Extensions.data();

    uint32 count_layers = 0;
    err = vkEnumerateInstanceLayerProperties( &count_layers, nullptr );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to enumerate Vulkan Instance layer properties. "
            "vkEnumerateInstanceLayerProperties() returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return VK_NULL_HANDLE;
    }

    std::vector<VkLayerProperties> available_layers( count_layers );
    err = vkEnumerateInstanceLayerProperties( &count_layers, available_layers.data() );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to enumerate Vulkan Instance layer properties. "
            "vkEnumerateInstanceLayerProperties() returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return VK_NULL_HANDLE;
    }

    for ( const auto& layer : ContextInfo.Layers )
    {
        if ( !IsLayerAvailable( available_layers, layer ) )
        {
            LOG_ERROR( "Required layer is not available. Layer name: %s", layer );
            return VK_NULL_HANDLE;
        }
    }

    instance_info.enabledLayerCount = static_cast< uint32 >( ContextInfo.Layers.size() );
    instance_info.ppEnabledLayerNames = ContextInfo.Layers.data();

    VkInstance instance;
    err = vkCreateInstance( &instance_info, nullptr, &instance );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Instance. vkCreateInstance() returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return VK_NULL_HANDLE;
    }

    return instance;
}

VkSurfaceKHR VulkanContext::CreateSurface()
{
    VkSurfaceKHR surface;
    if ( !SDL_Vulkan_CreateSurface( WindowHandle, Instance, nullptr, &surface ) )
    {
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Surface with SDL_Vulkan_CreateSurface. Error message: %s",
            SDL_GetError()
        );
        return VK_NULL_HANDLE;
    }
    return surface;
}

VkPhysicalDevice VulkanContext::SelectPhysicalDevice()
{
    VkResult err;

    uint32 count = 0;
    err = vkEnumeratePhysicalDevices( Instance, &count, nullptr );
    if ( err != VK_SUCCESS || count == 0 )
    {
        LOG_ERROR(
            "[Vulkan] Failed to enumerate GPUs with Vulkan support. vkEnumeratePhysicalDevices returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return VK_NULL_HANDLE;
    }

    if ( count == 0 )
    {
        throw std::runtime_error( "failed to find GPUs with Vulkan support" );
    }

    std::vector<VkPhysicalDevice> devices( count );
    err = vkEnumeratePhysicalDevices( Instance, &count, devices.data() );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to enumerate GPUs with Vulkan support. vkEnumeratePhysicalDevices returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return VK_NULL_HANDLE;
    }

    for ( const auto& device : devices )
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties( device, &props );

        //VkPhysicalDeviceFeatures features;
        //vkGetPhysicalDeviceFeatures( device, &features );

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

QueueFamilyIndices VulkanContext::FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    uint32 count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &count, nullptr );
    std::vector<VkQueueFamilyProperties> props( count );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &count, props.data() );

    QueueFamilyIndices indices;
    int32 family_index = 0;
    for ( const auto& queue_family : props )
    {
        if ( queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            indices.Graphics = family_index;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR( device, family_index, surface, &present_support );

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

VkDevice VulkanContext::CreateDevice( QueueFamilyIndices indices )
{
    std::vector<VkDeviceQueueCreateInfo> queue_infos = {};
    std::set<uint32> unique_families = {
        indices.Graphics.value(),
        indices.Present.value()
    };

    float priority = 1.0f;
    for ( uint32 family : unique_families )
    {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        queue_infos.push_back( queue_info );
    }

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = static_cast< uint32 >( queue_infos.size() );
    device_info.pQueueCreateInfos = queue_infos.data();

    VkPhysicalDeviceFeatures physical_device_features = {};
    device_info.pEnabledFeatures = &physical_device_features;
    device_info.enabledLayerCount = static_cast< uint32 >( ContextInfo.Layers.size() );
    device_info.ppEnabledLayerNames = ContextInfo.Layers.data();

    // the extension requires check for availability but i don't really care since i got rtx4060
    std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    device_info.enabledExtensionCount = static_cast< uint32 >( device_extensions.size() );
    device_info.ppEnabledExtensionNames = device_extensions.data();

    VkDevice device;
    VkResult err = vkCreateDevice( PhysicalDevice, &device_info, nullptr, &device );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Device. vkCreateDevice returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );

        return VK_NULL_HANDLE;
    }

    return device;
}

VkQueue VulkanContext::GetQueue( uint32 family_index, uint32 index )
{
    VkQueue queue;
    vkGetDeviceQueue( Device, family_index, index, &queue );
    return queue;
}

VulkanSwapchain VulkanContext::CreateSwapchain()
{
    VkResult err;

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = Surface;

    int32 width, height;
    SDL_GetWindowSize( WindowHandle, &width, &height );

    swapchain_info.imageExtent = { static_cast< uint32 >( width ), static_cast< uint32 >( height ) };

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
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Swapchain. vkCreateSwapchainKHR returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        swapchain.Instance = VK_NULL_HANDLE;
        return swapchain;
    }

    uint32 count_images = 0;
    err = vkGetSwapchainImagesKHR( Device, swapchain.Instance , &count_images, nullptr );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to receive Swapchain Images. vkGetSwapchainImagesKHR() returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
    }
    swapchain.Images.resize( count_images );

    err = vkGetSwapchainImagesKHR( Device, swapchain.Instance , &count_images, swapchain.Images.data() );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to receive swapchain images. vkgetswapchainimageskhr() returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
    }

    return swapchain;
}

void VulkanContext::CleanupSwapchain()
{
    const VkAllocationCallbacks* ALLOC = nullptr;

    for ( auto framebuffer : Framebuffers )
    {
        vkDestroyFramebuffer( Device, framebuffer, ALLOC );
    }

    for ( auto image_view : ImageViews )
    {
        vkDestroyImageView( Device, image_view, ALLOC );
    }

    vkDestroySwapchainKHR( Device, Swapchain.Instance, ALLOC );
}

void VulkanContext::RecreateSwapchain()
{
    vkDeviceWaitIdle( Device );

    CleanupSwapchain();

    Swapchain = CreateSwapchain();
    if ( Swapchain.Instance == VK_NULL_HANDLE )
    {
        // LOG_ERROR
    }

    ImageViews = CreateImageViews();
    if ( ImageViews.size() == 0 )
    {
        // LOG_ERROR
    }

    Framebuffers = CreateFramebuffers();
    if ( Framebuffers.size() == 0 )
    {
        // LOG_ERROR
    }
}

VkShaderModule VulkanContext::CreateShaderModule( const std::vector<char>& code )
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
            err 
        );
        return VK_NULL_HANDLE;
    }
    return shader_module;
}

VulkanGraphicsPipeline VulkanContext::CreateGraphicsPipeline( VkShaderModule vertex, VkShaderModule fragment )
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
        // LOG_ERROR
        return {};
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &graphics_pipeline.DescriptorSetLayout;

    err = vkCreatePipelineLayout( Device, &pipeline_layout_info, alloc, &graphics_pipeline.Layout );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( 
            "[Vulkan] Failed to create Vulkan Pipeline Layout."
            "vkCreatePipelineLayout returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return {};
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

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &subpass_dependency;

    err = vkCreateRenderPass( Device, &render_pass_info, alloc, &graphics_pipeline.RenderPass );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Render Pass. vkCreateRenderPass returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return {};
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

    auto binding_desc   = Vertex::GetBindingDescription();
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
    pipeline_info.layout = graphics_pipeline.Layout;
    pipeline_info.renderPass = graphics_pipeline.RenderPass;

	const VkPipelineCache        pipeline_cache = VK_NULL_HANDLE;
    const uint32                 create_count = 1;
    err = vkCreateGraphicsPipelines( Device, pipeline_cache, create_count, &pipeline_info, alloc,
        &graphics_pipeline.Instance );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Graphics Pipeline. vkCreateGraphicsPipeline returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return {};
    }


    return graphics_pipeline;
}


std::vector<VkImageView> VulkanContext::CreateImageViews()
{
    std::vector<VkImageView> image_views( Swapchain.Images.size() );

    for ( size_t i = 0; i < Swapchain.Images.size(); ++i )
    {
        VkImageViewCreateInfo image_view_info = {};
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.image = Swapchain.Images[i];
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = Swapchain.Format;
        image_view_info.components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        image_view_info.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        const VkAllocationCallbacks* ALLOC = nullptr;

        VkResult err = vkCreateImageView( Device, &image_view_info, ALLOC, &image_views[i] );
        if ( err != VK_SUCCESS )
        {
            LOG_ERROR(
                "[Vulkan] Failed to create Vulkan Image View. vkCreateImageView returned: {}={}",
                VK_TYPE_TO_STR( VkResult ),
                err 
            );
            return {};
        }
    }

    return image_views;
}

std::vector<VkFramebuffer> VulkanContext::CreateFramebuffers()
{
    std::vector<VkFramebuffer> framebuffers( ImageViews.size() );
    for ( size_t i = 0; i < ImageViews.size(); i++ )
    {
        VkImageView attachments[] = {
            ImageViews[i]
        };

        VkFramebufferCreateInfo framebuffer_info = {};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = GraphicsPipeline.RenderPass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = Swapchain.Extent.width;
        framebuffer_info.height = Swapchain.Extent.height;
        framebuffer_info.layers = 1;

        VkResult err = vkCreateFramebuffer( Device, &framebuffer_info, nullptr, &framebuffers[i] );
        if ( err != VK_SUCCESS )
        {
            LOG_ERROR(
                "[Vulkan] Failed to create framebuffer. vkCreateFramebuffer returned: {}={}",
                VK_TYPE_TO_STR( VkResult ),
                err 
            );
            return {};
        }
    }
    return framebuffers;
}

VkCommandPool VulkanContext::CreateCommandPool( QueueFamilyIndices indices )
{
    VkCommandPoolCreateInfo cmdpool_info = {};
    cmdpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdpool_info.queueFamilyIndex = indices.Graphics.value();

    VkCommandPool command_pool;
    VkResult err = vkCreateCommandPool( Device, &cmdpool_info, nullptr, &command_pool );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Command Pool. vkCreateCommandPool returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return VK_NULL_HANDLE;
    }
    return command_pool;
}

std::vector<VkCommandBuffer> VulkanContext::CreateCommandBuffers()
{
    std::vector<VkCommandBuffer> command_buffers( MAX_FRAMES_IN_FLIGHT );

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = CommandPool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast< uint32 >( command_buffers.size() );

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

VulkanSyncObjects VulkanContext::CreateSyncObjects()
{
    VkResult err;
    VulkanSyncObjects sync_objs;
    sync_objs.ImageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    sync_objs.RenderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    sync_objs.InFlightFences.resize( MAX_FRAMES_IN_FLIGHT );

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    const VkAllocationCallbacks* allocator = nullptr;

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        err = vkCreateSemaphore( Device, &semaphore_info, allocator, &sync_objs.ImageAvailableSemaphores[i] );
        if ( err != VK_SUCCESS )
        {
            // LOG_ERROR
            return {};
        }

        err = vkCreateSemaphore( Device, &semaphore_info, allocator, &sync_objs.RenderFinishedSemaphores[i] );
        if ( err != VK_SUCCESS )
        {
            // LOG_ERROR
            return {};
        }

        err = vkCreateFence( Device, &fence_info, allocator, &sync_objs.InFlightFences[i] );
        if ( err != VK_SUCCESS )
        {
            // LOG_ERROR
          return {};
        }
    }
    return sync_objs;
}

uint32 VulkanContext::FindMemoryType( uint32 type_filter, VkMemoryPropertyFlags prop_flags )
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

VulkanBuffer VulkanContext::CreateBuffer( VkDeviceSize size, VkBufferUsageFlags usage,
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
    if (err != VK_SUCCESS)
    {
        // LOG_ERROR
        return {};
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
		// LOG_ERROR
		return {};
	}

	const VkDeviceSize memory_offset = 0;
	vkBindBufferMemory( Device, buffer.Instance, buffer.Memory, memory_offset );

	return buffer;
}

VulkanBuffer VulkanContext::CreateVertexBuffer()
{
	const VkDeviceSize    buffer_size = sizeof( VERTICES[0] ) * VERTICES.size();
	VkBufferUsageFlags    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VulkanBuffer staging_buffer = CreateBuffer( buffer_size, usage, props );

	const VkDeviceSize     offset = 0;
	const VkMemoryMapFlags flags = 0;
	void* data;
	vkMapMemory( Device, staging_buffer.Memory, offset, buffer_size, flags, &data );
	memcpy( data, VERTICES.data(), static_cast< size_t >( buffer_size ) );
	vkUnmapMemory( Device, staging_buffer.Memory );

    usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VulkanBuffer vertex_buffer = CreateBuffer( buffer_size, usage, props );

    CopyBuffer( staging_buffer.Instance, vertex_buffer.Instance, buffer_size );

    const VkAllocationCallbacks* alloc = nullptr;
    vkDestroyBuffer( Device, staging_buffer.Instance, alloc );
    vkFreeMemory( Device, staging_buffer.Memory, alloc );

    return vertex_buffer;
}

VulkanBuffer VulkanContext::CreateIndexBuffer()
{
    VkDeviceSize buffer_size = sizeof( INDICES[0] ) * INDICES.size();

    VkBufferUsageFlags    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VulkanBuffer staging_buffer = CreateBuffer( buffer_size, usage, props );

    const VkDeviceSize     offset = 0;
    const VkMemoryMapFlags flags = 0;
    void* data;
    vkMapMemory( Device, staging_buffer.Memory, offset, buffer_size, flags, &data );
    memcpy( data, INDICES.data(), static_cast< size_t >( buffer_size ) );
    vkUnmapMemory( Device, staging_buffer.Memory );

    usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VulkanBuffer index_buffer = CreateBuffer( buffer_size, usage, props );

    CopyBuffer( staging_buffer.Instance, index_buffer.Instance, buffer_size );

    const VkAllocationCallbacks* alloc = nullptr;
    vkDestroyBuffer( Device, staging_buffer.Instance, alloc );
    vkFreeMemory( Device, staging_buffer.Memory, alloc );

    return index_buffer;
}

std::vector<VulkanBuffer> VulkanContext::CreateUniformBuffers()
{
    std::vector<VulkanBuffer> uniform_buffers( MAX_FRAMES_IN_FLIGHT );

    VkDeviceSize buffer_size = sizeof( UniformBufferObject );
    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
		VkBufferUsageFlags buffer_usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		uniform_buffers[i] = CreateBuffer( buffer_size, buffer_usage_flags, props );

        VkDeviceSize offset = 0;
		VkMemoryMapFlags memory_map_flags = 0;
		vkMapMemory( Device, uniform_buffers[i].Memory, offset, buffer_size, memory_map_flags, &uniform_buffers[i].Mapped );
    }

    return uniform_buffers;
}

void VulkanContext::CopyBuffer( VkBuffer source, VkBuffer destination, VkDeviceSize size )
{
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();

    VkBufferCopy buffer_copy = {};
    buffer_copy.size = size;
    const uint32 region_count = 1;
    vkCmdCopyBuffer( command_buffer, source, destination, region_count, &buffer_copy );

    EndSingleTimeCommands( command_buffer );
}

void VulkanContext::UpdateUniformBuffer( uint32 current_image )
{
    static auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>( current_time - start_time ).count();
    
    UniformBufferObject ubo = {};
    ubo.Model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
    ubo.View = glm::lookAt( glm::vec3( 2.0f ), glm::vec3( 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
    ubo.Projection = glm::perspective( glm::radians( 45.0f ),
        Swapchain.Extent.width / static_cast< float >( Swapchain.Extent.height ), 0.1f, 10.0f );
    ubo.Projection[1][1] *= -1;

	memcpy( UniformBuffers[current_image].Mapped, &ubo, sizeof( ubo ) );
}

VulkanDescriptorGroup VulkanContext::CreateDescriptorGroup()
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
        // LOG_ERROR
		return { .Pool = VK_NULL_HANDLE };
    }

    std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT, GraphicsPipeline.DescriptorSetLayout );
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = descriptor_group.Pool;
    allocate_info.descriptorSetCount = static_cast< uint32 >( MAX_FRAMES_IN_FLIGHT );
    allocate_info.pSetLayouts = layouts.data();

    descriptor_group.Sets.resize( MAX_FRAMES_IN_FLIGHT );
    err = vkAllocateDescriptorSets( Device, &allocate_info, descriptor_group.Sets.data() );
    if ( err != VK_SUCCESS )
    {
        // LOG_ERROR
		return { .Pool = VK_NULL_HANDLE };
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
        vkUpdateDescriptorSets( Device, descriptor_write_count, descriptor_writes.data(), descriptor_copy_count,
            descriptor_copies );
    }

    return descriptor_group;
}

Vulkan::Texture VulkanContext::CreateTexture()
{
    VkResult err;

    auto assets_path = Application::ExecutablePath() / ".." / ".." / ".." / ".." / "Assets" / "brick.jpg";
    std::string assets_path_string = assets_path.string();

    int32 width;
    int32 height;
    int32 channels;
	stbi_uc* pixels = stbi_load( assets_path_string.c_str(), &width, &height, &channels, STBI_rgb_alpha );
    if ( !pixels )
    {
		LOG_ERROR( "not loaded {}", assets_path_string );
        return { VK_NULL_HANDLE };
    }

    VkDeviceSize image_size = width * height * 4;

    VkBufferUsageFlags    flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VulkanBuffer staging_buffer = CreateBuffer( image_size, flags, props );

    const VkDeviceSize offset = 0;
    const VkMemoryMapFlags memory_flags = 0;
    void* data;
    err = vkMapMemory( Device, staging_buffer.Memory, offset, image_size, memory_flags, &data );
	if ( err != VK_SUCCESS )
	{
        // LOG_ERROR
    }

    memcpy( data, pixels, static_cast< size_t >( image_size ) );
    vkUnmapMemory( Device, staging_buffer.Memory );

    stbi_image_free( pixels );

    flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    Vulkan::Texture texture = CreateTextureImage( width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		flags, props );

    VkImageLayout old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    TransitionImageLayout( texture, VK_FORMAT_R8G8B8A8_SRGB, old_layout, new_layout );
    CopyBufferToImage( 
        staging_buffer.Instance,
        texture.Image, 
        static_cast< uint32 >( width ), 
        static_cast< uint32 >( height ) 
    );

    old_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	TransitionImageLayout( texture, VK_FORMAT_R8G8B8A8_SRGB, old_layout, new_layout );

    const VkAllocationCallbacks* alloc = nullptr;
    vkDestroyBuffer( Device, staging_buffer.Instance, alloc );
    vkFreeMemory( Device, staging_buffer.Memory, alloc );

    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = texture.Image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_SRGB;

    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount = 1;

    view_info.subresourceRange = subresource_range;

    err = vkCreateImageView( Device, &view_info, alloc, &texture.View );
    if ( err != VK_SUCCESS )
    {
        // LOG_ERROR
    }

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
        // LOG_ERROR
    }

    return texture;
}

Vulkan::Texture VulkanContext::CreateTextureImage( int32 width, int32 height, VkFormat format, VkImageTiling tiling, 
    VkImageUsageFlags usage, VkMemoryPropertyFlags memory_props )
{
    VkResult err;
    Vulkan::Texture texture;
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
        return { VK_NULL_HANDLE };
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
        // LOG_ERROR
        return { VK_NULL_HANDLE };
    }

    const VkDeviceSize memory_offset = 0;
	vkBindImageMemory( Device, texture.Image, texture.Memory, memory_offset );

    return texture;
}

void VulkanContext::RecordCommandBuffer( uint32 image_index )
{
	VkResult err;
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	err = vkBeginCommandBuffer( CommandBuffers[CurrentFrame], &begin_info );
	if ( err != VK_SUCCESS )
	{
		LOG_ERROR(
			"[Vulkan] Error beginning recording Vulkan Command Buffer. vkBeginCommandBuffer returned: {}={}",
			VK_TYPE_TO_STR( VkResult ),
			err
		);
		throw std::runtime_error( "failed to begin recording command buffer" );
	}

	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = GraphicsPipeline.RenderPass;
    render_pass_info.framebuffer = Framebuffers[image_index];
    render_pass_info.renderArea.offset = { 0,0 };
    render_pass_info.renderArea.extent = Swapchain.Extent;

    VkClearValue color = { {{ 0.0f, 0.0f, 0.0f, 1.0f }} };
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &color;
    
    vkCmdBeginRenderPass( CommandBuffers[CurrentFrame], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline( CommandBuffers[CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline.Instance);

    VkBuffer vertex_buffers[] = { VertexBuffer.Instance };
    VkDeviceSize offsets[] = { 0 };

    const uint32 FIRST_BINDING = 0;
    const uint32 BINDING_COUNT = 1;
	vkCmdBindVertexBuffers( CommandBuffers[CurrentFrame], FIRST_BINDING, BINDING_COUNT, vertex_buffers, offsets );

    const VkDeviceSize offset = 0;
    vkCmdBindIndexBuffer( CommandBuffers[CurrentFrame], IndexBuffer.Instance, offset, VK_INDEX_TYPE_UINT16 );

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast< float >( Swapchain.Extent.width );
    viewport.height = static_cast< float >( Swapchain.Extent.height );
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport( CommandBuffers[CurrentFrame], 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = { 0,0 };
    scissor.extent = Swapchain.Extent;
    vkCmdSetScissor( CommandBuffers[CurrentFrame], 0, 1, &scissor);

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

    err = vkEndCommandBuffer( CommandBuffers[CurrentFrame]);
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Error ending recording Vulkan Command Buffer. vkEndCommandBuffer returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        throw std::runtime_error( "failed to end recording command buffer" );
    }
}

VkCommandBuffer VulkanContext::BeginSingleTimeCommands()
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
        // LOG_ERROR
    }

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	err = vkBeginCommandBuffer( command_buffer, &begin_info );
    if ( err != VK_SUCCESS )
    {
        // LOG_ERROR
    }

    return command_buffer;
}

void VulkanContext::EndSingleTimeCommands( VkCommandBuffer command_buffer )
{
    vkEndCommandBuffer( command_buffer );

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    const VkFence fence = VK_NULL_HANDLE;
    vkQueueSubmit( GraphicsQueue, 1, &submit_info, fence );
    vkQueueWaitIdle( GraphicsQueue );

    const uint32 command_buffer_count = 1;
    vkFreeCommandBuffers( Device, CommandPool, command_buffer_count, &command_buffer );
}

void VulkanContext::TransitionImageLayout( const Vulkan::Texture& texture, VkFormat format, VkImageLayout old_layout,
    VkImageLayout new_layout )
{
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();

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
    if ( old_layout == VK_IMAGE_LAYOUT_UNDEFINED && 
         new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
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
    else
    {
        throw std::runtime_error( "unsupported layout transition" );
    }

    vkCmdPipelineBarrier( 
        command_buffer, 
        source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier );

    EndSingleTimeCommands( command_buffer );
}

void VulkanContext::CopyBufferToImage( VkBuffer buffer, VkImage image, uint32 width, uint32 height )
{
    VkCommandBuffer command_buffer = BeginSingleTimeCommands();

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

    EndSingleTimeCommands(command_buffer);
}

