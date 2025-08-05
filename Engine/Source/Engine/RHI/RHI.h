// src/Engine/Renderer/Core/GraphicsContext.h

#ifndef __renderer_graphics_context_h_included__
#define __renderer_graphics_context_h_included__

#include "Engine/Core/Common.h"

class RHIContext
{
public:
	enum class Backend
	{
		Vulkan,
		D3D12,
		OpenGL
	};

	virtual ~RHIContext() = default;

	virtual void Init() = 0;

	virtual void BeginFrame() = 0;
	virtual void DrawFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void Cleanup() = 0;

	virtual void SwapBuffers() = 0;

	static Scope<RHIContext> Create( void* window, Backend backend );
};

#endif 