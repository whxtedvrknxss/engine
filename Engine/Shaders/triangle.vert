#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Projection;
} ubo;

layout(location = 0) in vec2 InPosition;
layout(location = 1) in vec3 InColor;
layout(location = 2) in vec2 InTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.Projection * ubo.View * ubo.Model * vec4(InPosition, 0.0, 1.0);
    fragColor = InColor;
    fragTexCoord = InTexCoord;
}

