// Engine/src/Core/Window.h

#ifndef __window_h_included__
#define __window_h_included__

#include <string>
#include <memory>

struct WindowCreateInfo {
  uint32_t Width;
  uint32_t Height;
  std::string Title;
};

class Window {
public:
  Window( const Window &window ) = delete;
  virtual ~Window() = default;
  
  virtual void OnUpdate() = 0;

  virtual void* GetNativeWindow() const = 0;
 
  static std::unique_ptr<Window> Create( const WindowCreateInfo& create_info );
};

#endif 