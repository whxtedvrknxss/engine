// src/Engine/Core/Assert.h

#ifndef __assert_h_included__
#define __assert_h_included__

#include "Common.h"

#include <cassert>

  #ifdef ENABLE_ASSERT

    #define ASSERT(expression) assert(expression)

  #else

    #define ASSERT(expression) (void)0

  #endif 

#endif
