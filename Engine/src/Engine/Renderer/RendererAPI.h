// src/Engine/Renderer/RendererAPI.h

#ifndef __renderer_renderer_api_h_included__
#define __renderer_renderer_api_h_included__

class RendererAPI {
public:
  enum class API {
    None      = 0,
    Vulkan    = 1,
    OpenGL    = 2,
    DirectX12 = 3
  };

public:
  virtual ~RendererAPI() = default;
};

#endif 