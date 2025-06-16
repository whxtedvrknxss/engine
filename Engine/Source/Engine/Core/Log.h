
#ifndef __log_h_included__
#define __log_h_included__

#include <format>

#include <SDL3/SDL_log.h>

#define LOG_INFO(...)               SDL_Log(std::format(__VA_ARGS__).c_str())
#define LOG_ERROR(...)              SDL_Log(std::format(__VA_ARGS__).c_str())

#endif 
