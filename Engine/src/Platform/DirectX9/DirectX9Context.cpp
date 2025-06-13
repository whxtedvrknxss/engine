#include "DirectX9Context.h"

DirectX9Context::DirectX9Context(SDL_Window* window_handle) {
    WindowHandle = window_handle;
}

void DirectX9Context::Init() {
    Device->Release();
}
