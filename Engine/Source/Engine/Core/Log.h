
#ifndef __log_h_included__
#define __log_h_included__

#include <format>

#include <SDL3/SDL_log.h>

#define LOG_INFO(...)               std::printf( std::format("[INFO] {}\n", std::format(__VA_ARGS__)).c_str() )
#define LOG_ERROR(...)              std::printf( std::format("[ERROR] {}\n", std::format(__VA_ARGS__)).c_str() )

#include <vulkan/vulkan.h>
#include <string>

template<>
struct std::formatter<VkResult> : std::formatter<std::string>
{
    auto format( VkResult result, auto& ctx ) const
    {
        std::string str;
        switch ( result )
        {
            case VK_SUCCESS: str = "VK_SUCCESS"; break;
            case VK_NOT_READY: str = "VK_NOT_READY"; break;
            case VK_TIMEOUT: str = "VK_TIMEOUT"; break;
            case VK_EVENT_SET: str = "VK_EVENT_SET"; break;
            case VK_EVENT_RESET: str = "VK_EVENT_RESET"; break;
            case VK_INCOMPLETE: str = "VK_INCOMPLETE"; break;
            case VK_ERROR_OUT_OF_HOST_MEMORY: str = "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: str = "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
            case VK_ERROR_INITIALIZATION_FAILED: str = "VK_ERROR_INITIALIZATION_FAILED"; break;
            case VK_ERROR_DEVICE_LOST: str = "VK_ERROR_DEVICE_LOST"; break;
            case VK_ERROR_MEMORY_MAP_FAILED: str = "VK_ERROR_MEMORY_MAP_FAILED"; break;
            case VK_ERROR_LAYER_NOT_PRESENT: str = "VK_ERROR_LAYER_NOT_PRESENT"; break;
            case VK_ERROR_EXTENSION_NOT_PRESENT: str = "VK_ERROR_EXTENSION_NOT_PRESENT"; break;
            case VK_ERROR_FEATURE_NOT_PRESENT: str = "VK_ERROR_FEATURE_NOT_PRESENT"; break;
            case VK_ERROR_INCOMPATIBLE_DRIVER: str = "VK_ERROR_INCOMPATIBLE_DRIVER"; break;
            case VK_ERROR_TOO_MANY_OBJECTS: str = "VK_ERROR_TOO_MANY_OBJECTS"; break;
            case VK_ERROR_FORMAT_NOT_SUPPORTED: str = "VK_ERROR_FORMAT_NOT_SUPPORTED"; break;
            case VK_ERROR_SURFACE_LOST_KHR: str = "VK_ERROR_SURFACE_LOST_KHR"; break;
            case VK_SUBOPTIMAL_KHR: str = "VK_SUBOPTIMAL_KHR"; break;
            case VK_ERROR_OUT_OF_DATE_KHR: str = "VK_ERROR_OUT_OF_DATE_KHR"; break;
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: str = "VK_ERROR_INCOMPATIBLE_DRIVER"; break;
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: str = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break;
            case VK_ERROR_VALIDATION_FAILED_EXT: str = "VK_ERROR_VALIDATION_FAILED_EXT"; break;
            default: str = "UNKNOWN_VKRESULT(" + std::to_string( static_cast< int >( result ) ) + ")";
        }
        return std::formatter<std::string>::format( str, ctx );
    }
};


#endif 
