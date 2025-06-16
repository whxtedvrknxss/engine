// src/Engine/Renderer/Core/GraphicsContext.h

#ifndef __renderer_graphics_context_h_included__
#define __renderer_graphics_context_h_included__

#include "RendererAPI.h"
#include "Engine/Core/Common.h"

class GraphicsContext {
public:
  virtual ~GraphicsContext() = default;

  virtual void Init() = 0;

  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;
  virtual void Cleanup() = 0;

  virtual void SwapBuffers() = 0;

  static Scope<GraphicsContext> Create( void* window, GraphicsBackend api );
};

#endif 