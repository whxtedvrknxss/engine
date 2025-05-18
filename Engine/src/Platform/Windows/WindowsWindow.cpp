#include "WindowsWindow.h"

WindowsWindow::WindowsWindow( const WindowCreateInfo& create_info ) {
  m_Data.Width = create_info.Width;
  m_Data.Height = create_info.Height;
  m_Data.Title = create_info.Title;

  m_Window = glfwCreateWindow( m_Data.Width, m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr );

}

WindowsWindow::~WindowsWindow() {

}
