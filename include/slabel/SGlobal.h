#pragma once

#if defined(_WIN32)
#  if defined(SLABEL_BUILD)
#    define SLABEL_EXPORT __declspec(dllexport)
#  else
#    define SLABEL_EXPORT __declspec(dllimport)
#  endif
#else
#  define SLABEL_EXPORT __attribute__((visibility("default")))
#endif

// 库版本号
SLABEL_EXPORT const char* slabelVersion();
