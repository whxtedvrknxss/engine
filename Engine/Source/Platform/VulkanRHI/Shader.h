// Platform/Vulkan/Shader.h

#ifndef __renderer_vulkan_shader_h_included__
#define __renderer_vulkan_shader_h_included__

#include <vector>
#include <filesystem>

#include <vulkan/vulkan_core.h>

std::vector<char> GetShaderSource( const std::filesystem::path& path );

#endif 