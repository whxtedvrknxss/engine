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
    VertexModule = VK_NULL_HANDLE;
    FragmentModule = VK_NULL_HANDLE;
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
    if ( Swapchain.Handle == VK_NULL_HANDLE || Swapchain.Images.size() == 0 )
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
    if ( VertexModule == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "VertexModule == VK_NULL_HANDLE" );
    }

    VkShaderModule fragment = CreateShaderModule( fragment_shader );
    if ( FragmentModule == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "FragmentModule == VK_NULL_HANDLE" );
    }

    CreateGraphicsPipeline( vertex, fragment);

    vkDestroyShaderModule( Device, vertex, nullptr );
    vkDestroyShaderModule( Device, fragment, nullptr );
}

void VulkanContext::Cleanup()
{
    for ( auto image_view : ImageViews )
    {
        vkDestroyImageView( Device, image_view, nullptr );
    }

    vkDestroySwapchainKHR( Device, Swapchain.Handle, nullptr );
    vkDestroyDevice( Device, nullptr );
    vkDestroySurfaceKHR( Instance, Surface, nullptr );
    vkDestroyInstance( Instance, nullptr );
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
            "vkEnumerateInstanceExtensionProperties returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
        );
        return VK_NULL_HANDLE;
    }

    std::vector<VkExtensionProperties> available_ext( count_ext );
    err = vkEnumerateInstanceExtensionProperties( nullptr, &count_ext, available_ext.data() );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to enumerate Vulkan Instance extension properties. "
            "vkEnumerateInstanceExtensionProperties returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
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
            "vkEnumerateInstanceLayerProperties() returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
        );
        return VK_NULL_HANDLE;
    }

    std::vector<VkLayerProperties> available_layers( count_layers );
    err = vkEnumerateInstanceLayerProperties( &count_layers, available_layers.data() );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to enumerate Vulkan Instance layer properties. "
            "vkEnumerateInstanceLayerProperties() returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
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
            "[Vulkan] Failed to create Vulkan Instance. vkCreateInstance() returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
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
            "[Vulkan] Failed to enumerate GPUs with Vulkan support. vkEnumeratePhysicalDevices returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
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
            "[Vulkan] Failed to enumerate GPUs with Vulkan support. vkEnumeratePhysicalDevices returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
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

    QueueFamilyIndices indices;
    int32 i = 0;
    for ( const auto& queue_family : props )
    {
        if ( queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            indices.Graphics = i;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &present_support );

        if ( present_support )
        {
            indices.Present = i;
        }

        if ( indices.IsComplete() )
        {
            break;
        }

        ++i;
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
            "[Vulkan] Failed to create Vulkan Device. vkCreateDevice returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
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
    swapchain_info.imageFormat = VK_FORMAT_R8G8B8_UNORM;
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
    err = vkCreateSwapchainKHR( Device, &swapchain_info, nullptr, &swapchain.Handle );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Swapchain. vkCreateSwapchainKHR returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
        );
        swapchain.Handle = VK_NULL_HANDLE;
        return swapchain;
    }

    uint32 count_images = 0;
    err = vkGetSwapchainImagesKHR( Device, swapchain.Handle, &count_images, nullptr );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to receive Swapchain Images. vkGetSwapchainImagesKHR() returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
        );
        swapchain.Images.resize( count_images );
    }

    err = vkGetSwapchainImagesKHR( Device, swapchain.Handle, &count_images, swapchain.Images.data() );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to receive swapchain images. vkgetswapchainimageskhr() returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
        );
    }

    return swapchain;
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

        VkResult err = vkCreateImageView( Device, &image_view_info, nullptr, &ImageViews[i] );
        if ( err != VK_SUCCESS )
        {
            LOG_ERROR(
                "[Vulkan] Failed to create Vulkan Image View. vkCreateImageView returned: %s=%d",
                VK_TYPE_TO_STR( VkResult ),
                static_cast< int32 >( err )
            );
            return {};
        }
    }

    return image_views;
}

VkShaderModule VulkanContext::CreateShaderModule( const std::vector<char>& code )
{
    VkShaderModuleCreateInfo module_info = {};
    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = code.size();
    module_info.pCode = reinterpret_cast< const uint32_t* >( code.data() );

    VkShaderModule shader_module;
    VkResult err = vkCreateShaderModule( Device, &module_info, nullptr, &shader_module );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR(
            "[Vulkan] Failed to create Vulkan Shader Module. vkCreateShaderModule returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            static_cast< int32 >( err )
        );
        return VK_NULL_HANDLE;
    }
    return shader_module;
}

void VulkanContext::CreateGraphicsPipeline( VkShaderModule vertex, VkShaderModule fragment )
{
    VkPipelineShaderStageCreateInfo vertex_info = {};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_info.module = vertex;
    vertex_info.pName = "main";

    VkPipelineShaderStageCreateInfo fragment_info = {};
    fragment_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_info.module = fragment;
    fragment_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_infos[] = { vertex_info, fragment_info };

    std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = static_cast< uint32 >( dynamic_states.size() );
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    VkPipelineInputAssemblyStateCreateInfo assembly_info = {};
    assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assembly_info.primitiveRestartEnable = VK_FALSE;

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
    colorblend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorblend_info = {};
    colorblend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorblend_info.attachmentCount = 1;
    colorblend_info.pAttachments = &colorblend_attachment;
    
    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkPipelineLayout pipeline_layout;
    VkResult err = vkCreatePipelineLayout( Device, &pipeline_layout_info, nullptr, &pipeline_layout );
    if ( err != VK_SUCCESS )
    {
        LOG_ERROR( 
            "[Vulkan] Failed to create Vulkan Pipeline Layout."
            "vkCreatePipelineLayout returned: %s=%d",
            VK_TYPE_TO_STR( VkResult ),
            err
        );
        return VK_NULL_HANDLE;
    }
    
    
}

