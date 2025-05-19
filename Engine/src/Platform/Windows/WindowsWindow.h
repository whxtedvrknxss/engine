// src/Platform/WindowsWindow.h

#ifndef __windows_window_h_included__
#define __windows_window_h_included__

#include "Engine/Core/Window.h"
#include "Engine/Core/Common.h"

struct GLFWwindow;

class WindowsWindow : public Window {
public:
  WindowsWindow() = delete;
  WindowsWindow( const WindowCreateInfo& create_info );
  ~WindowsWindow() override;
  
	void OnUpdate() override;
  virtual void* GetNativeWindow() const { return m_Window; }

private:
  GLFWwindow* m_Window;

  struct {
    Vec2<uint32_t> Position;
    Vec2<uint32_t> Size;
    std::string    Title;  
    bool           VSync;
  } m_Data;
};

#endif
