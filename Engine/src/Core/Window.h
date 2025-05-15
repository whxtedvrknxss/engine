// Engine/src/Core/Window.h

#ifndef __window_h_included__
#define __window_h_included__

#include <string>

struct WindowProps {
  uint32_t Width;
  uint32_t Height;
  std::string Title;

  WindowProps(uint32_t width = 800,
              uint32_t height = 800,
              const std::string &title = "useful engine")
    : Width(width), Height(height), Title(title) {}
};

class Window {
  Window() = delete;
  Window( const Window &window ) = delete;

 
  static std::unique_ptr<Window> Create( const WindowProps &props );
};

#endif 