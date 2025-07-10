// Engine/src/Core/Application.h

#ifndef __application_h_included__
#define __application_h_included__

#include <memory>

#include "Window.h"
#include "Common.h"
#include "Engine/Renderer/GraphicsContext.h"

class Application
{
public:
    Application();
    ~Application();

    int32 Run();

private:
    Scope<WindowBase> Window;
};

#endif 