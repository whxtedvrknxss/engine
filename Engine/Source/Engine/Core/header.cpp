//
//// Dear ImGui: standalone example application for SDL3 + Vulkan
//
//// Learn about Dear ImGui:
//// - FAQ                  https://dearimgui.com/faq
//// - Getting Started      https://dearimgui.com/getting-started
//// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
//// - Introduction, links and more at the top of imgui.cpp
//
//// Important note to the reader who wish to integrate imgui_impl_vulkan.cpp/.h in their own engine/app.
//// - Common ImGui_ImplVulkan_XXX functions and structures are used to interface with imgui_impl_vulkan.cpp/.h.
////   You will use those if you want to use this rendering backend in your engine/app.
//// - Helper ImGui_ImplVulkanH_XXX functions and structures are only used by this example (main.cpp) and by
////   the backend itself (imgui_impl_vulkan.cpp), but should PROBABLY NOT be used by your own engine/app code.
//// Read comments in imgui_impl_vulkan.h.
//
//#include "header.h"
//
//#include <imgui_internal.h>
//
//#include "imgui.h"
//#include <stdio.h>          // printf, fprintf
//#include <stdlib.h>         // abort
//#include <vector>
//#include <backends/imgui_impl_sdl3.h>
//#include <backends/imgui_impl_vulkan.h>
//#include <misc/freetype/imgui_freetype.h>
//#include <SDL3/SDL.h>
//#include <SDL3/SDL_vulkan.h>
//
//// This example doesn't compile with Emscripten yet! Awaiting SDL3 support.
//#ifdef __EMSCRIPTEN__
//#include "../libs/emscripten/emscripten_mainloop_stub.h"
//#endif
//
//// Volk headers
//#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
//#define VOLK_IMPLEMENTATION
//#include <volk.h>
//#endif
//
//#define APP_USE_UNLIMITED_FRAME_RATE
//#ifdef _DEBUG
//#define APP_USE_VULKAN_DEBUG_REPORT
//static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
//#endif
//
//// Data
//static VkAllocationCallbacks* g_Allocator = nullptr;
//static VkInstance               g_Instance = VK_NULL_HANDLE;
//static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
//static VkDevice                 g_Device = VK_NULL_HANDLE;
//static uint32_t                 g_QueueFamily = ( uint32_t ) -1;
//static VkQueue                  g_Queue = VK_NULL_HANDLE;
//static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
//static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;
//
//static ImGui_ImplVulkanH_Window g_MainWindowData;
//static uint32_t                 g_MinImageCount = 2;
//static bool                     g_SwapChainRebuild = false;
//
//static void check_vk_result( VkResult err )
//{
//    if ( err == VK_SUCCESS )
//        return;
//    fprintf( stderr, "[vulkan] Error: VkResult = %d\n", err );
//    if ( err < 0 )
//        abort();
//}
//
//#ifdef APP_USE_VULKAN_DEBUG_REPORT
//static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData )
//{
//    ( void ) flags; ( void ) object; ( void ) location; ( void ) messageCode; ( void ) pUserData; ( void ) pLayerPrefix; // Unused arguments
//    fprintf( stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage );
//    return VK_FALSE;
//}
//#endif // APP_USE_VULKAN_DEBUG_REPORT
//
//static bool IsExtensionAvailable( const ImVector<VkExtensionProperties>& properties, const char* extension )
//{
//    for ( const VkExtensionProperties& p : properties )
//        if ( strcmp( p.extensionName, extension ) == 0 )
//            return true;
//    return false;
//}
//
//static void SetupVulkan( ImVector<const char*> instance_extensions )
//{
//    VkResult err;
//#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
//    volkInitialize();
//#endif
//
//    // Create Vulkan Instance
//    {
//        VkInstanceCreateInfo create_info = {};
//        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//
//        // Enumerate available extensions
//        uint32_t properties_count;
//        ImVector<VkExtensionProperties> properties;
//        vkEnumerateInstanceExtensionProperties( nullptr, &properties_count, nullptr );
//        properties.resize( properties_count );
//        err = vkEnumerateInstanceExtensionProperties( nullptr, &properties_count, properties.Data );
//        check_vk_result( err );
//
//        // Enable required extensions
//        if ( IsExtensionAvailable( properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME ) )
//            instance_extensions.push_back( VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );
//#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
//        if ( IsExtensionAvailable( properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME ) )
//        {
//            instance_extensions.push_back( VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );
//            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
//        }
//#endif
//
//        // Enabling validation layers
//#ifdef APP_USE_VULKAN_DEBUG_REPORT
//        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
//        create_info.enabledLayerCount = 1;
//        create_info.ppEnabledLayerNames = layers;
//        instance_extensions.push_back( "VK_EXT_debug_report" );
//#endif
//
//        // Create Vulkan Instance
//        create_info.enabledExtensionCount = ( uint32_t ) instance_extensions.Size;
//        create_info.ppEnabledExtensionNames = instance_extensions.Data;
//        err = vkCreateInstance( &create_info, g_Allocator, &g_Instance );
//        check_vk_result( err );
//#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
//        volkLoadInstance( g_Instance );
//#endif
//
//        // Setup the debug report callback
//#ifdef APP_USE_VULKAN_DEBUG_REPORT
//        auto f_vkCreateDebugReportCallbackEXT = ( PFN_vkCreateDebugReportCallbackEXT ) vkGetInstanceProcAddr( g_Instance, "vkCreateDebugReportCallbackEXT" );
//        IM_ASSERT( f_vkCreateDebugReportCallbackEXT != nullptr );
//        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
//        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
//        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
//        debug_report_ci.pfnCallback = debug_report;
//        debug_report_ci.pUserData = nullptr;
//        err = f_vkCreateDebugReportCallbackEXT( g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport );
//        check_vk_result( err );
//#endif
//    }
//
//    // Select Physical Device (GPU)
//    g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice( g_Instance );
//    IM_ASSERT( g_PhysicalDevice != VK_NULL_HANDLE );
//
//    // Select graphics queue family
//    g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex( g_PhysicalDevice );
//    IM_ASSERT( g_QueueFamily != ( uint32_t ) -1 );
//
//    // Create Logical Device (with 1 queue)
//    {
//        ImVector<const char*> device_extensions;
//        device_extensions.push_back( "VK_KHR_swapchain" );
//
//        // Enumerate physical device extension
//        uint32_t properties_count;
//        ImVector<VkExtensionProperties> properties;
//        vkEnumerateDeviceExtensionProperties( g_PhysicalDevice, nullptr, &properties_count, nullptr );
//        properties.resize( properties_count );
//        vkEnumerateDeviceExtensionProperties( g_PhysicalDevice, nullptr, &properties_count, properties.Data );
//#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
//        if ( IsExtensionAvailable( properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME ) )
//            device_extensions.push_back( VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME );
//#endif
//
//        const float queue_priority[] = { 1.0f };
//        VkDeviceQueueCreateInfo queue_info[1] = {};
//        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//        queue_info[0].queueFamilyIndex = g_QueueFamily;
//        queue_info[0].queueCount = 1;
//        queue_info[0].pQueuePriorities = queue_priority;
//        VkDeviceCreateInfo create_info = {};
//        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//        create_info.queueCreateInfoCount = sizeof( queue_info ) / sizeof( queue_info[0] );
//        create_info.pQueueCreateInfos = queue_info;
//        create_info.enabledExtensionCount = ( uint32_t ) device_extensions.Size;
//        create_info.ppEnabledExtensionNames = device_extensions.Data;
//        err = vkCreateDevice( g_PhysicalDevice, &create_info, g_Allocator, &g_Device );
//        check_vk_result( err );
//        vkGetDeviceQueue( g_Device, g_QueueFamily, 0, &g_Queue );
//    }
//
//    // Create Descriptor Pool
//    // If you wish to load e.g. additional textures you may need to alter pools sizes and maxSets.
//    {
//        VkDescriptorPoolSize pool_sizes[] =
//        {
//            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE },
//        };
//        VkDescriptorPoolCreateInfo pool_info = {};
//        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
//        pool_info.maxSets = 0;
//        for ( VkDescriptorPoolSize& pool_size : pool_sizes )
//            pool_info.maxSets += pool_size.descriptorCount;
//        pool_info.poolSizeCount = ( uint32_t ) IM_ARRAYSIZE( pool_sizes );
//        pool_info.pPoolSizes = pool_sizes;
//        err = vkCreateDescriptorPool( g_Device, &pool_info, g_Allocator, &g_DescriptorPool );
//        check_vk_result( err );
//    }
//}
//
//// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
//// Your real engine/app may not use them.
//static void SetupVulkanWindow( ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height )
//{
//    wd->Surface = surface;
//
//    // Check for WSI support
//    VkBool32 res;
//    vkGetPhysicalDeviceSurfaceSupportKHR( g_PhysicalDevice, g_QueueFamily, wd->Surface, &res );
//    if ( res != VK_TRUE )
//    {
//        fprintf( stderr, "Error no WSI support on physical device 0\n" );
//        exit( -1 );
//    }
//
//    // Select Surface Format
//    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
//    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
//    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat( g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, ( size_t ) IM_ARRAYSIZE( requestSurfaceImageFormat ), requestSurfaceColorSpace );
//
//    // Select Present Mode
//#ifdef APP_USE_UNLIMITED_FRAME_RATE
//    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
//#else
//    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
//#endif
//    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode( g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE( present_modes ) );
//    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);
//
//    // Create SwapChain, RenderPass, Framebuffer, etc.
//    IM_ASSERT( g_MinImageCount >= 2 );
//    ImGui_ImplVulkanH_CreateOrResizeWindow( g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount );
//}
//
//static void CleanupVulkan()
//{
//    vkDestroyDescriptorPool( g_Device, g_DescriptorPool, g_Allocator );
//
//#ifdef APP_USE_VULKAN_DEBUG_REPORT
//    // Remove the debug report callback
//    auto f_vkDestroyDebugReportCallbackEXT = ( PFN_vkDestroyDebugReportCallbackEXT ) vkGetInstanceProcAddr( g_Instance, "vkDestroyDebugReportCallbackEXT" );
//    f_vkDestroyDebugReportCallbackEXT( g_Instance, g_DebugReport, g_Allocator );
//#endif // APP_USE_VULKAN_DEBUG_REPORT
//
//    vkDestroyDevice( g_Device, g_Allocator );
//    vkDestroyInstance( g_Instance, g_Allocator );
//}
//
//static void CleanupVulkanWindow()
//{
//    ImGui_ImplVulkanH_DestroyWindow( g_Instance, g_Device, &g_MainWindowData, g_Allocator );
//}
//
//static void FrameRender( ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data )
//{
//    VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
//    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
//    VkResult err = vkAcquireNextImageKHR( g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex );
//    if ( err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR )
//        g_SwapChainRebuild = true;
//    if ( err == VK_ERROR_OUT_OF_DATE_KHR )
//        return;
//    if ( err != VK_SUBOPTIMAL_KHR )
//        check_vk_result( err );
//
//    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
//    {
//        err = vkWaitForFences( g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX );    // wait indefinitely instead of periodically checking
//        check_vk_result( err );
//
//        err = vkResetFences( g_Device, 1, &fd->Fence );
//        check_vk_result( err );
//    }
//    {
//        err = vkResetCommandPool( g_Device, fd->CommandPool, 0 );
//        check_vk_result( err );
//        VkCommandBufferBeginInfo info = {};
//        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//        err = vkBeginCommandBuffer( fd->CommandBuffer, &info );
//        check_vk_result( err );
//    }
//    {
//        VkRenderPassBeginInfo info = {};
//        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
//        info.renderPass = wd->RenderPass;
//        info.framebuffer = fd->Framebuffer;
//        info.renderArea.extent.width = wd->Width;
//        info.renderArea.extent.height = wd->Height;
//        info.clearValueCount = 1;
//        info.pClearValues = &wd->ClearValue;
//        vkCmdBeginRenderPass( fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE );
//    }
//
//    // Record dear imgui primitives into command buffer
//    ImGui_ImplVulkan_RenderDrawData( draw_data, fd->CommandBuffer );
//
//    // Submit command buffer
//    vkCmdEndRenderPass( fd->CommandBuffer );
//    {
//        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//        VkSubmitInfo info = {};
//        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//        info.waitSemaphoreCount = 1;
//        info.pWaitSemaphores = &image_acquired_semaphore;
//        info.pWaitDstStageMask = &wait_stage;
//        info.commandBufferCount = 1;
//        info.pCommandBuffers = &fd->CommandBuffer;
//        info.signalSemaphoreCount = 1;
//        info.pSignalSemaphores = &render_complete_semaphore;
//
//        err = vkEndCommandBuffer( fd->CommandBuffer );
//        check_vk_result( err );
//        err = vkQueueSubmit( g_Queue, 1, &info, fd->Fence );
//        check_vk_result( err );
//    }
//}
//
//static void FramePresent( ImGui_ImplVulkanH_Window* wd )
//{
//    if ( g_SwapChainRebuild )
//        return;
//    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
//    VkPresentInfoKHR info = {};
//    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//    info.waitSemaphoreCount = 1;
//    info.pWaitSemaphores = &render_complete_semaphore;
//    info.swapchainCount = 1;
//    info.pSwapchains = &wd->Swapchain;
//    info.pImageIndices = &wd->FrameIndex;
//    VkResult err = vkQueuePresentKHR( g_Queue, &info );
//    if ( err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR )
//        g_SwapChainRebuild = true;
//    if ( err == VK_ERROR_OUT_OF_DATE_KHR )
//        return;
//    if ( err != VK_SUBOPTIMAL_KHR )
//        check_vk_result( err );
//    wd->SemaphoreIndex = ( wd->SemaphoreIndex + 1 ) % wd->SemaphoreCount; // Now we can use the next set of semaphores
//}
//
//// Main code
//void show_window()
//{
//    // Setup SDL
//    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
//    if ( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMEPAD ) )
//    {
//        printf( "Error: SDL_Init(): %s\n", SDL_GetError() );
//        return;
//    }
//
//    // Create window with Vulkan graphics context
//    SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
//    SDL_Window* window = SDL_CreateWindow( "Dear ImGui SDL3+Vulkan example", 1280, 720, window_flags );
//    if ( window == nullptr )
//    {
//        printf( "Error: SDL_CreateWindow(): %s\n", SDL_GetError() );
//        return;
//    }
//
//    ImVector<const char*> extensions;
//    {
//        uint32_t sdl_extensions_count = 0;
//        const char* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions( &sdl_extensions_count );
//        for ( uint32_t n = 0; n < sdl_extensions_count; n++ )
//            extensions.push_back( sdl_extensions[n] );
//    }
//    SetupVulkan( extensions );
//
//    std::vector<int> a;
//
//    // Create Window Surface
//    VkSurfaceKHR surface;
//    VkResult err;
//    if ( SDL_Vulkan_CreateSurface( window, g_Instance, g_Allocator, &surface ) == 0 )
//    {
//        printf( "Failed to create Vulkan surface.\n" );
//        return;
//    }
//
//    // Create Framebuffers
//    int w, h;
//    SDL_GetWindowSize( window, &w, &h );
//    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
//    SetupVulkanWindow( wd, surface, w, h );
//    SDL_SetWindowPosition( window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED );
//    SDL_ShowWindow( window );
//
//    // Setup Dear ImGui context
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO& io = ImGui::GetIO(); ( void ) io;
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
//
//    ImFontConfig font_config;
//    font_config.OversampleH = 3;
//    font_config.OversampleV = 3;
//    font_config.PixelSnapH = false;
//
//    io.Fonts->AddFontFromFileTTF( "c:\\windows\\fonts\\lucon.ttf", 15.0f, &font_config );
//
//    // Setup Dear ImGui style
//    ImGui::StyleColorsDark();
//    //ImGui::StyleColorsLight();
//
//    // Setup Platform/Renderer backends
//    ImGui_ImplSDL3_InitForVulkan( window );
//    ImGui_ImplVulkan_InitInfo init_info = {};
//    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
//    init_info.Instance = g_Instance;
//    init_info.PhysicalDevice = g_PhysicalDevice;
//    init_info.Device = g_Device;
//    init_info.QueueFamily = g_QueueFamily;
//    init_info.Queue = g_Queue;
//    init_info.PipelineCache = g_PipelineCache;
//    init_info.DescriptorPool = g_DescriptorPool;
//    init_info.RenderPass = wd->RenderPass;
//    init_info.Subpass = 0;
//    init_info.MinImageCount = g_MinImageCount;
//    init_info.ImageCount = wd->ImageCount;
//    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
//    init_info.Allocator = g_Allocator;
//    init_info.CheckVkResultFn = check_vk_result;
//    ImGui_ImplVulkan_Init( &init_info );
//
//    // Load Fonts
//    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
//    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
//    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
//    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
//    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
//    // - Read 'docs/FONTS.md' for more instructions and details.
//    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
//    //io.Fonts->AddFontDefault();
//    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
//    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
//    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
//    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
//    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
//    //IM_ASSERT(font != nullptr);
//
//    // Our state
//    ImVec4 clear_color = ImVec4( 0.45f, 0.55f, 0.60f, 1.00f );
//
//    // Main loop
//    bool done = false;
//    while ( !done )
//    {
//        // Poll and handle events (inputs, window resize, etc.)
//        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
//        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
//        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
//        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
//        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
//        SDL_Event event;
//        while ( SDL_PollEvent( &event ) )
//        {
//            ImGui_ImplSDL3_ProcessEvent( &event );
//            if ( event.type == SDL_EVENT_QUIT )
//                done = true;
//            if ( event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID( window ) )
//                done = true;
//        }
//
//        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
//        if ( SDL_GetWindowFlags( window ) & SDL_WINDOW_MINIMIZED )
//        {
//            SDL_Delay( 10 );
//            continue;
//        }
//
//        // Resize swap chain?
//        int fb_width, fb_height;
//        SDL_GetWindowSize( window, &fb_width, &fb_height );
//        if ( fb_width > 0 && fb_height > 0 && ( g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height ) )
//        {
//            ImGui_ImplVulkan_SetMinImageCount( g_MinImageCount );
//            ImGui_ImplVulkanH_CreateOrResizeWindow( g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount );
//            g_MainWindowData.FrameIndex = 0;
//            g_SwapChainRebuild = false;
//        }
//
//        // Start the Dear ImGui frame
//        ImGui_ImplVulkan_NewFrame();
//        ImGui_ImplSDL3_NewFrame();
//        ImGui::NewFrame();
//
//        if ( ImGui::BeginMainMenuBar() )
//        {
//            if ( ImGui::BeginMenu( "File" ) )
//            {
//                if ( ImGui::MenuItem( "New" ) ) { /* Handle New */ }
//                if ( ImGui::MenuItem( "Open...", "Ctrl+O" ) ) { /* Handle Open */ }
//                if ( ImGui::MenuItem( "Save", "Ctrl+S" ) ) { /* Handle Save */ }
//                if ( ImGui::MenuItem( "Exit" ) ) { /* Handle Exit */ }
//                ImGui::EndMenu();
//            }
//
//            if ( ImGui::BeginMenu( "Edit" ) )
//            {
//                if ( ImGui::MenuItem( "Undo", "Ctrl+Z" ) ) {}
//                if ( ImGui::MenuItem( "Redo", "Ctrl+Y" ) ) {}
//                ImGui::Separator();
//                if ( ImGui::MenuItem( "Cut", "Ctrl+X" ) ) {}
//                if ( ImGui::MenuItem( "Copy", "Ctrl+C" ) ) {}
//                if ( ImGui::MenuItem( "Paste", "Ctrl+V" ) ) {}
//                ImGui::EndMenu();
//            }
//
//            if ( ImGui::BeginMenu( "Help" ) )
//            {
//                if ( ImGui::MenuItem( "About" ) ) {}
//                ImGui::EndMenu();
//            }
//
//            ImGui::EndMainMenuBar();
//        }
//
//
//        static bool opt_fullscreen = true;
//        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoWindowMenuButton;
//
//        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar |
//            ImGuiWindowFlags_NoDocking;
//        if ( opt_fullscreen )
//        {
//            ImGuiViewport* viewport = ImGui::GetMainViewport();
//            ImGui::SetNextWindowPos( viewport->Pos );
//            ImGui::SetNextWindowSize( viewport->Size );
//            ImGui::SetNextWindowViewport( viewport->ID );
//
//            ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
//            ImGui::PushStyleVar( ImGuiStyleVar_WindowBorderSize, 0.0f );
//            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
//
//            window_flags |= ImGuiWindowFlags_NoTitleBar |
//                ImGuiWindowFlags_NoCollapse |
//                ImGuiWindowFlags_NoResize |
//                ImGuiWindowFlags_NoMove |
//                ImGuiWindowFlags_NoBringToFrontOnFocus |
//                ImGuiWindowFlags_NoNavFocus;
//        }
//
//        ImGui::Begin( "DockSpace Demo", nullptr, window_flags );
//
//        if ( opt_fullscreen )
//            ImGui::PopStyleVar( 3 );
//
//        // DockSpace
//        ImGuiID dockspace_id = ImGui::GetID( "MyDockSpace" );
//        ImGui::DockSpace( dockspace_id, ImVec2( 0.0f, 0.0f ), dockspace_flags );
//
//        ImGui::End();
//
//        // Example windows docked inside
//        ImGui::Begin( "Sidebar" );
//        ImGui::Text( "Text 1" );
//        ImGui::Text( "Text 2" );
//        ImGui::Text( "Text 3" );
//        ImGui::End();
//
//        ImGui::Begin( "Content" );
//        ImGui::ShowDemoWindow();
//        ImGui::End();
//
//        // Rendering
//        ImGui::Render();
//        ImDrawData* draw_data = ImGui::GetDrawData();
//        const bool is_minimized = ( draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f );
//        if ( !is_minimized )
//        {
//            wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
//            wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
//            wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
//            wd->ClearValue.color.float32[3] = clear_color.w;
//            FrameRender( wd, draw_data );
//            FramePresent( wd );
//        }
//
//        if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
//        {
//            ImGui::UpdatePlatformWindows();
//            ImGui::RenderPlatformWindowsDefault();
//            // TODO for OpenGL: restore current GL context.
//        }
//    }
//
//    // Cleanup
//    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
//    err = vkDeviceWaitIdle( g_Device );
//    check_vk_result( err );
//    ImGui_ImplVulkan_Shutdown();
//    ImGui_ImplSDL3_Shutdown();
//    ImGui::DestroyContext();
//
//    CleanupVulkanWindow();
//    CleanupVulkan();
//
//    SDL_DestroyWindow( window );
//    SDL_Quit();
//}

#include "header.h"

#include "Application.h"

void show_window( int argc, char* argv[] )
{
	Application app( argc, argv );
    app.Run();
}   
