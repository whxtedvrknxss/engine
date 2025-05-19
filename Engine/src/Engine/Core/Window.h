// Engine/src/Core/Window.h

#ifndef __window_h_included__
#define __window_h_included__

#include <string>
#include <memory>

#include "Engine/Core/Common.h"

struct WindowCreateInfo {
  Vec2<uint32_t> Position;
  Vec2<uint32_t> Size;
  std::string    Title;
};

class Window {
public:
  virtual ~Window() = default;
  
  virtual void OnUpdate() = 0;

  virtual void* GetNativeWindow() const = 0;
 
  static std::unique_ptr<Window> Create( const WindowCreateInfo& create_info );
};

#endif 