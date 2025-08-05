// Engine/src/Core/Application.h

#ifndef __application_h_included__
#define __application_h_included__

#include <memory>
#include <filesystem>

#include "Window.h"
#include "Common.h"

class Application
{
public:
	Application( int argc, char* argv[] );
    ~Application();

    int32 Run();

    static inline std::filesystem::path ExecutablePath()
    {
        return ExePath;
    }

private:
    Scope<WindowBase> Window;
    static std::filesystem::path ExePath;
};


#endif 