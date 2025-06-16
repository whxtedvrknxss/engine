// Engine/src/Core/Window.h

#ifndef __window_h_included__
#define __window_h_included__

#include <string>
#include <memory>

#include "Engine/Core/Common.h"

#include <SDL3/SDL.h>

struct WindowCreateInfo {
  Vec2<uint32_t> Position;
  Vec2<uint32_t> Size;
  std::string    Title;
};

class WindowBase {
public:
  virtual ~WindowBase() = default;
  
  virtual void OnUpdate() = 0;

  virtual void* GetNativeWindow() const = 0;
 
  static std::unique_ptr<WindowBase> Create( const WindowCreateInfo& create_info );
};

#endif 