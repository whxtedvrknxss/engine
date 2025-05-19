#include "WindowsWindow.h"

#include <GLFW/glfw3.h>

WindowsWindow::WindowsWindow( const WindowCreateInfo& create_info ) {
  m_Data.Position = create_info.Position;
  m_Data.Size = create_info.Size;
  m_Data.Title = create_info.Title;

  m_Window = glfwCreateWindow( m_Data.Size.X, m_Data.Size.Y, m_Data.Title.c_str(), 
    nullptr, nullptr );
}

WindowsWindow::~WindowsWindow() {
  glfwDestroyWindow( m_Window );
}

void WindowsWindow::OnUpdate() {
  glfwSwapBuffers( m_Window );
}

