// src/Engine/Renderer/RendererAPI.h

#ifndef __renderer_renderer_api_h_included__
#define __renderer_renderer_api_h_included__

enum class GraphicsBackend {
    None = 0,
    Vulkan,
    DirectX12,
    OpenGL,
};

class RendererAPI {
public:
  virtual ~RendererAPI() = default;
};

#endif 