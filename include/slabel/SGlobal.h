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

// 弃用标记：库自身构建（定义 SLABEL_BUILD）时为空，避免内部编译产生噪声警告；
// 仅外部使用者在引用被标记符号时看到 deprecated 编译期警告。
#if defined(SLABEL_BUILD)
#  define SLABEL_DEPRECATED(msg)
#else
#  define SLABEL_DEPRECATED(msg) [[deprecated(msg)]]
#endif

// 库版本号
SLABEL_EXPORT const char* slabelVersion();
