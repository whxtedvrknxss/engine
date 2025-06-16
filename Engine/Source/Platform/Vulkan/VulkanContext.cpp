#include "VulkanContext.h"

#include <stdexcept>

#include "Engine/Core/Common.h"

#include <SDL3/SDL_vulkan.h>

VulkanContext::VulkanContext( const VulkanContextCreateInfo& context_info, SDL_Window* window_handle ) {
    WindowHandle = window_handle;
}
