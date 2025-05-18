// src/Platform/WindowsWindow.h

#ifndef __windows_window_h_included__
#define __windows_window_h_included__

#include <GLFW/glfw3.h>

#include "Engine/Core/Window.h"

class WindowsWindow : public Window {
public:
  WindowsWindow() = delete;
  WindowsWindow( const WindowCreateInfo& create_info );
  ~WindowsWindow() override;
  
	void OnUpdate() override;
  void* GetNativeWindow() const override;

private:
  GLFWwindow* m_Window;

  struct {
    uint32_t Width;
    uint32_t Height;
    std::string Title;  
    bool VSync;
  } m_Data;
};

#endif
