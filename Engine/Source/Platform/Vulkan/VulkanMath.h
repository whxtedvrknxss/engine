// Source/Platform/Vulkan/VulkanMath.h

#ifndef __renderer_vulkan_math_h_included__
#define __renderer_vulkan_math_h_included__

#include <array>

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

struct Vertex
{
    glm::vec2 Pos;
    glm::vec3 Color;
    glm::vec2 TexCoord;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription description = {};
        description.binding = 0;
        description.stride = sizeof( Vertex );
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return description;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 3> description {};

        description[0].binding = 0;
        description[0].location = 0;
        description[0].format = VK_FORMAT_R32G32_SFLOAT;
        description[0].offset = offsetof( Vertex, Pos );

        description[1].binding = 0;
        description[1].location = 1;
        description[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        description[1].offset = offsetof( Vertex, Color );

        description[2].binding = 0;
        description[2].location = 2;
        description[2].format = VK_FORMAT_R32G32_SFLOAT;
        description[2].offset = offsetof( Vertex, TexCoord );

        return description;
    }
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Projection;
};

const std::vector<Vertex> VERTICES = {
    {{-0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }},
    {{ 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }},
    {{ 0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }},
    {{-0.5f,  0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f }}
};

const std::vector<uint16> INDICES = {
	0, 1, 2, 2, 3, 0
};

#endif 
