// src/Platform/Vulkan/VulkanCall.h
#ifndef __renderer_vulkan_call_h_included__
#define __renderer_vulkan_call_h_included__

#include <SDL3/SDL_log.h>
#include <vulkan/vulkan.h>

#include "Engine/Core/Common.h"
#include "Engine/Core/Assert.h"

#define VK_CALL(fn) \
            do { if ( VkResult res = fn; res != VK_SUCCESS ) { \
                    SDL_Log( "Vulkan Error: %d at %s:%d", res, __FILE__, __LINE__ );   \
                    ASSERT( false && "Vulkan call failed!" ); \
                }} while (0) 

#endif 
