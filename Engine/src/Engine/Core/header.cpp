#include "header.h"

#include <string>
#include <iostream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Assert.h"

void show_window() {
  glfwInit();

  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
  GLFWwindow *window = glfwCreateWindow( 800, 600, "vulkan window", nullptr, nullptr );

  glfwSetDropCallback( window, [] ( GLFWwindow* window, int count, const char* paths[] ) {
    for ( int i = 0; i < count; i++ ) {
      std::printf( "%s\n", paths[i] );
      glfwSetClipboardString( window, paths[i] );
    }
  });

  glfwShowWindow( window );

  ASSERT( false && "gabela ahaahhah" );

  glm::mat4 matrix;
  glm::vec4 vec;
  auto test = matrix * vec;

  while ( !glfwWindowShouldClose( window ) ) {
    glfwPollEvents();
  }

  glfwDestroyWindow( window );
  glfwTerminate();
}

