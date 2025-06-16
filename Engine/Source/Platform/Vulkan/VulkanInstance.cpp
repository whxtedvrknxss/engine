#include "VulkanInstance.h"

#include <cassert>
#include <span>
#include <stdexcept>

#include <SDL3/SDL_vulkan.h>

#include "Engine/Core/Common.h"
#include "Engine/Core/Assert.h"

#include "VulkanCall.h"
#include "VulkanContext.h"

void VulkanInstance::Create( const VulkanContextCreateInfo& context_info ) {
    // TODO:
    // implement VulkanContextCreateInfo optional logic 

    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = context_info.ApplicationName;
    application_info.applicationVersion = VK_MAKE_VERSION( 0, 0, 0 );
    application_info.pEngineName = context_info.EngineName;
    application_info.engineVersion = VK_MAKE_VERSION( 0, 0, 0 );
    application_info.apiVersion = VK_MAKE_API_VERSION( 0, context_info.VulkanApiMajorVersion,
        context_info.VulkanApiMinorVersion, 0 );

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &application_info;

    if ( !CheckRequiredExtensionSupport( context_info.Extensions ) ) {
        throw std::runtime_error( "required extensions are not supported" );
    }

    instance_info.enabledExtensionCount = STATIC_CAST( uint32, context_info.Extensions.size() );
    instance_info.ppEnabledExtensionNames = context_info.Extensions.data();

    if ( !CheckRequiredLayerSupport( context_info.Layers ) ) {
        throw std::runtime_error( "required layers are not supported" );
    }

    instance_info.enabledLayerCount = STATIC_CAST( uint32, context_info.Layers.size() );
    instance_info.ppEnabledLayerNames = context_info.Layers.data();

    VK_CALL( vkCreateInstance( &instance_info, nullptr, &Instance ) );
}

void VulkanInstance::Cleanup() {
    vkDestroyInstance( Instance, nullptr );
}

bool VulkanInstance::CheckRequiredExtensionSupport( const std::vector<const char*>& required ) {
    uint32 available_count = 0;
    VK_CALL( vkEnumerateInstanceExtensionProperties( nullptr, &available_count, nullptr ) );

    VkExtensionProperties* available = nullptr;
    VK_CALL( vkEnumerateInstanceExtensionProperties( nullptr, &available_count, available ) );

    for ( const char* r : required ) {
        bool found = false;
        for ( uint32 i = 0; i < available_count; ++i ) {
            if ( strcmp( r, available[i].extensionName ) == 0 ) {
                found = true;
                break;
            }
        }

        if ( !found ) {
            return false;
        }
    }

    return true;
}

bool VulkanInstance::CheckRequiredLayerSupport( const std::vector<const char*>& required ) {
    uint32 available_count = 0;
    VK_CALL( vkEnumerateInstanceLayerProperties( &available_count, nullptr ) );

    VkLayerProperties* available = nullptr;
    VK_CALL( vkEnumerateInstanceLayerProperties( &available_count, available ) );

    for ( const char* r : required ) {
        bool found = false;
        for ( uint32 i = 0; i < available_count; ++i ) {
            if ( strcmp( r, available[i].layerName ) == 0 ) {
                found = true;
            }
        }
        
        if ( !found ) {
            return false;
        }
    }

    return true;
}

