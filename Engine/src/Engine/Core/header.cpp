#include "header.h"

#include <string>
#include <iostream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FOCE_DEPTH_ZERO
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

void show_window() {
  glfwInit();

  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
  GLFWwindow *window = glfwCreateWindow( 800, 600, "vulkan window", nullptr, nullptr );

  uint32_t extension_count = 0;
  vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, nullptr );

  std::vector<VkExtensionProperties> properties( extension_count );
  vkEnumerateInstanceExtensionProperties( nullptr, &extension_count, properties.data() );

  for ( int i = 0; i < extension_count; i++ ) {
    std::printf( "%s\n", properties[i].extensionName);
  }

  glfwShowWindow( window );

  auto str { std::to_string( extension_count ) };
  glfwSetWindowTitle( window, str.c_str() );

  glm::mat4 matrix;
  glm::vec4 vec;
  auto test = matrix * vec;

  while ( !glfwWindowShouldClose( window ) ) {
    glfwPollEvents();
  }

  glfwDestroyWindow( window );

  glfwTerminate();
}

