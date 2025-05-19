// src/Engine/Core/Common.h

#ifndef __common_h_included__
#define __common_h_included__

#define STATIC_CAST(type, value) static_cast<type>(value)

template< typename Type >
struct Vec2 {
  Type X, Y;
};

#endif 