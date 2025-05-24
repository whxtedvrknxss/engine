// Engine/src/Core/Application.h

#ifndef __application_h_included__
#define __application_h_included__

#include <memory>

#include "Window.h"
#include "Common.h"

class Application {
  Scope<Window> m_Window;

public:
  Application();


};

#endif 