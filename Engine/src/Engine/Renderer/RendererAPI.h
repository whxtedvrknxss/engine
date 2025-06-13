// src/Engine/Renderer/RendererAPI.h

#ifndef __renderer_renderer_api_h_included__
#define __renderer_renderer_api_h_included__

enum class GraphicsBackend {
    None      = 0,
    Vulkan    = 1,
    OpenGL    = 2,
    DirectX9  = 3,
    DirectX11 = 4,
    DirectX12 = 5,
};

class RendererAPI {
public:
  virtual ~RendererAPI() = default;
};

#endif 