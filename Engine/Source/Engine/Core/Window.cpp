#include "Window.h"

#if defined( PLATFORM_WINDOWS )
#include "Platform/Windows/WindowsWindow.h"
#endif 

std::unique_ptr<WindowBase> WindowBase::Create( const WindowCreateInfo& create_info )
{
#if defined( PLATFORM_WINDOWS )
    return std::make_unique<WindowsWindow>( create_info );
#endif
}
