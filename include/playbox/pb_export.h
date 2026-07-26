#ifndef PLAYBOX_PB_EXPORT_H
#define PLAYBOX_PB_EXPORT_H

#define PB_VERSION_MAJOR 1
#define PB_VERSION_MINOR 2
#define PB_VERSION_PATCH 0
#define PB_VERSION_STRING "1.2.0"

#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef PB_BUILD_SHARED
    #define PB_API __declspec(dllexport)
  #elif defined(PB_USE_SHARED)
    #define PB_API __declspec(dllimport)
  #else
    #define PB_API
  #endif
#else
  #if defined(PB_BUILD_SHARED) || defined(__GNUC__)
    #define PB_API __attribute__((visibility("default")))
  #else
    #define PB_API
  #endif
#endif

#endif
