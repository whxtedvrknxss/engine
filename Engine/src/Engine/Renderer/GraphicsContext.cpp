#include "GraphicsContext.h"

#include "RendererAPI.h"
#include "Platform/DirectX9/DirectX9Context.h"
#include "Platform/Vulkan/VulkanContext.h"

Scope<GraphicsContext> GraphicsContext::Create( void* window, GraphicsBackend api ) {
    switch ( api ) {
        case GraphicsBackend::Vulkan: {
            return CreateScope<VulkanContext>( STATIC_CAST( SDL_Window*, window ) );
        } break;

        case GraphicsBackend::DirectX9: {
            return CreateScope<DirectX9Context>( STATIC_CAST( SDL_Window*, window ) );
        } break;
    }

  return nullptr;
}
