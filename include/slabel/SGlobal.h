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
//
// 属性形式说明：本宏与 SLABEL_EXPORT 同时出现在类头
//   class SLABEL_EXPORT SLABEL_DEPRECATED("...") Name
// 在 GCC 上会展开成 __attribute__((visibility(...))) 后紧跟 [[deprecated]]，
// 而 gcc<9 不接受“GNU 属性后紧跟 C++11 标准属性”，会编译报错。
// 因此在 GCC/Clang 改用 GNU 风格 __attribute__((deprecated))（与 visibility 同为
// GNU 属性，可并列），MSVC 用 __declspec，其它编译器才回退到标准 [[deprecated]]。
#if defined(SLABEL_BUILD)
#  define SLABEL_DEPRECATED(msg)
#elif defined(_MSC_VER)
#  define SLABEL_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__) || defined(__clang__)
#  define SLABEL_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
#  define SLABEL_DEPRECATED(msg) [[deprecated(msg)]]
#endif

// 库版本号
SLABEL_EXPORT const char* slabelVersion();
