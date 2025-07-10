#include "VulkanContext.h"

#include <set>
#include <stdexcept>

#include <SDL3/SDL_vulkan.h>>

#include "Engine/Core/Common.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Assert.h"
#include "Shader.h"

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
    Swapchain = {};
    GraphicsPipeline = {};
    CommandPool = VK_NULL_HANDLE;
    CommandBuffer = VK_NULL_HANDLE;
    SyncObjects = {};
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
    if ( Framebuffers.size() == 0 )
    {
        throw std::runtime_error( "Framebuffers.size == 0" );
    }

    CommandPool = CreateCommandPool( indices );
    if ( CommandPool == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "CommandPool == VK_NULL_HANDLE" );
    }

    CommandBuffer = CreateCommandBuffer();
    if ( CommandBuffer == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "CommandBuffer == VK_NULL_HANDLE" );
    }

    SyncObjects = CreateSyncObjects();
    bool sync_objs_invalid = SyncObjects.ImageAvailableSemaphore == VK_NULL_HANDLE ||
                             SyncObjects.InFlightFence == VK_NULL_HANDLE || 
                             SyncObjects.RenderFinishedSemaphore == VK_NULL_HANDLE;
    if ( sync_objs_invalid )
    {
        throw std::runtime_error( "sync objects are invalid" );
    }
}

void VulkanContext::Cleanup()
{
    const VkAllocationCallbacks* ALLOCATOR = nullptr;

    vkDestroySemaphore( Device, SyncObjects.ImageAvailableSemaphore, ALLOCATOR );
    vkDestroySemaphore( Device, SyncObjects.RenderFinishedSemaphore, ALLOCATOR );
    vkDestroyFence( Device, SyncObjects.InFlightFence, ALLOCATOR );

    vkDestroyCommandPool( Device, CommandPool, ALLOCATOR );

    for ( auto framebuffer : Framebuffers )
    {
        vkDestroyFramebuffer( Device, framebuffer, ALLOCATOR );
    }

    vkDestroyPipeline( Device, GraphicsPipeline.Instance, ALLOCATOR );
    vkDestroyPipelineLayout( Device, GraphicsPipeline.Layout, ALLOCATOR );
    vkDestroyRenderPass( Device, GraphicsPipeline.RenderPass, ALLOCATOR );

    for ( auto image_view : ImageViews )
    {
        vkDestroyImageView( Device, image_view, ALLOCATOR );
    }

    vkDestroySwapchainKHR( Device, Swapchain.Instance, ALLOCATOR );
    vkDestroyDevice( Device, ALLOCATOR );
    vkDestroySurfaceKHR( Instance, Surface, ALLOCATOR );
    vkDestroyInstance( Instance, ALLOCATOR );
}

void VulkanContext::DrawFrame()
{
    VkResult err;

    const uint32 FENCE_COUNT = 1;
    const VkBool32 WAIT_ALL = VK_TRUE;
    const uint64 TIMEOUT = UINT64_MAX;
    err = vkWaitForFences( Device, FENCE_COUNT, &SyncObjects.InFlightFence, WAIT_ALL, TIMEOUT );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( "[Vulkan] Error from vkWaitForFences: {}", err );
    }

    err = vkResetFences( Device, FENCE_COUNT, &SyncObjects.InFlightFence );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( "[Vulkan] Error from vkResetFences: %d", err );
    }

    uint32 image_index;
    VkFence FENCE = VK_NULL_HANDLE;
    err = vkAcquireNextImageKHR( Device, Swapchain.Instance, TIMEOUT, SyncObjects.ImageAvailableSemaphore,
        FENCE, &image_index );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( "[Vulkan] Error from vkAcquireNextImageKHR: %d", err );
    }

    err = vkResetCommandBuffer( CommandBuffer, 0 );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( "[Vulkan] Error from vkResetCommandBuffer: %d", err );
    }
    RecordCommandBuffer( image_index );

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = { SyncObjects.ImageAvailableSemaphore };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &CommandBuffer;

    VkSemaphore signal_semaphores[] = { SyncObjects.RenderFinishedSemaphore };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    const uint32 SUBMIT_COUNT = 1;
    err = vkQueueSubmit( GraphicsQueue, SUBMIT_COUNT, &submit_info, SyncObjects.InFlightFence );
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

    vkQueuePresentKHR( PresentQueue, &present_info );
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

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    err = vkCreatePipelineLayout( Device, &pipeline_layout_info, nullptr, &graphics_pipeline.Layout );
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

    err = vkCreateRenderPass( Device, &render_pass_info, nullptr, &graphics_pipeline.RenderPass );
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

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;


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
    rasterizer_info.frontFace = VK_FRONT_FACE_CLOCKWISE;

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

    const VkPipelineCache        PIPELINE_CACHE = VK_NULL_HANDLE;
    const uint32                 CREATE_COUNT = 1;
    const VkAllocationCallbacks* ALLOC = nullptr;
    err = vkCreateGraphicsPipelines( Device, PIPELINE_CACHE, CREATE_COUNT, &pipeline_info, ALLOC, &graphics_pipeline.Instance );
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

VkCommandBuffer VulkanContext::CreateCommandBuffer()
{
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = CommandPool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    VkResult err = vkAllocateCommandBuffers( Device, &alloc_info, &command_buffer );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Error allocating Vulkan Command Buffer. vkAllocateCommandBuffers returned: {}={}",
            VK_TYPE_TO_STR( VkResult ),
            err 
        );
        return VK_NULL_HANDLE;
    }
    return command_buffer;
}

VulkanSyncObjects VulkanContext::CreateSyncObjects()
{
    VkResult err;
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    const VkAllocationCallbacks* allocator = nullptr;

    VulkanSyncObjects sync_objects;
    err = vkCreateSemaphore( Device, &semaphore_info, allocator, &sync_objects.ImageAvailableSemaphore );
    if ( err != VK_SUCCESS )
    {
        // LOG_ERROR
        return {};
    }

    err = vkCreateSemaphore( Device, &semaphore_info, allocator, &sync_objects.RenderFinishedSemaphore );
    if ( err != VK_SUCCESS )
    {
        // LOG_ERROR
        return {};
    }

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    err = vkCreateFence( Device, &fence_info, allocator, &sync_objects.InFlightFence );
    if ( err != VK_SUCCESS )
    {
        // LOG_ERROR
        return {};
    }
    return sync_objects;
}

void VulkanContext::RecordCommandBuffer( uint32 image_index )
{
    VkResult err;
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    err = vkBeginCommandBuffer( CommandBuffer, &begin_info );
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
    
    vkCmdBeginRenderPass( CommandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline.Instance );

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast< float >( Swapchain.Extent.width );
    viewport.height = static_cast< float >( Swapchain.Extent.height );
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport( CommandBuffer, 0, 1, &viewport );

    VkRect2D scissor = {};
    scissor.offset = { 0,0 };
    scissor.extent = Swapchain.Extent;
    vkCmdSetScissor( CommandBuffer, 0, 1, &scissor );

    const uint32 vertex_count = 3;
    const uint32 instance_count = 1;
    const uint32 first_vertex = 0;
    const uint32 first_instance = 0;
    vkCmdDraw( CommandBuffer, vertex_count, instance_count, first_vertex, first_instance );

    vkCmdEndRenderPass( CommandBuffer );

    err = vkEndCommandBuffer( CommandBuffer );
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

