#include "GraphicsContext.h"

#include <SDL3/SDL_vulkan.h>

#include "RendererAPI.h"
#include "Engine/Core/Assert.h"

#if defined( VULKAN_SUPPORTED )
#include "Platform/Vulkan/VulkanContext.h"
#endif 

Scope<GraphicsContext> GraphicsContext::Create( void* window, GraphicsBackend api ) {
    switch ( api ) {
#if defined ( VULKAN_SUPPORTED )
        case GraphicsBackend::Vulkan: {
            uint32 extensions_count = 0;
            char const* const* extensions = SDL_Vulkan_GetInstanceExtensions( &extensions_count );

            // TODO
            VulkanContextCreateInfo context_info = {
                .VulkanApiMajorVersion = 1,
                .VulkanApiMinorVersion = 2,
                .Extensions = std::vector( extensions, extensions + extensions_count ),
                .Layers = { "VK_LAYER_KHRONOS_validation" },
                .ApplicationName = "application_name",
                .EngineName = "engine_name" 
            };

            return CreateScope<VulkanContext>( context_info, STATIC_CAST( SDL_Window*, window ) );
        } break;
#endif

        default: {
            ASSERT( false, "Platform is not supported" );
        } break;
    }

  return nullptr;
}
