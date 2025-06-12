// src/Engine/Core/Common.h

#ifndef __core_common_h_included__
#define __core_common_h_included__

#include <memory>

#ifdef DEBUG
  #define ENABLE_ASSERT
#endif

#define STATIC_CAST(type, value)        static_cast<type>(value)

template< typename Type >
struct Vec2 {
  Type X, Y;
};

template<typename Type>
using Scope = std::unique_ptr<Type>;

template<typename Type, typename ...Args>
constexpr Scope<Type> CreateScope( Args&&... args ) {
  return std::make_unique<Type>( std::forward<Args>( args )... );
}

template<typename Type>
using Ref = std::shared_ptr<Type>;

template<typename Type, typename ...Args>
constexpr Ref<Type> CreateRef( Args&&... args ) {
  return std::make_shared<Type>( std::forward<Args>( args )... );
}

#endif 
