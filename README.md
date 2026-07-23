# standard_lable (slabel)

Qt 5.15 控件能力库：只提供统一可切换主题（颜色/字体/图标）、运行时多语言、双向/单向数据绑定、
操作状态反馈这些**机制**，不内置任何具体控件类型——业务层按需用 `SControl<Base>` 套自己
选的 Qt 控件类型即可接入全部能力，扩展新控件类型无需修改本库代码。

## 构建
CMake：`cmake -S . -B build && cmake --build build`
qmake：`qmake standard_lable.pro && make`

## 用法
```cpp
#include "slabel/SControl.h"
#include "slabel/ThemeManager.h"
#include <QPushButton>

ThemeManager::instance().registerTheme("dark", "themes/dark.qss");
ThemeManager::instance().setTheme("dark");

// 任意 Qt 控件类型套上 SControl<> 即获得主题/语言/操作状态能力，无需继承
auto* btn = new SControl<QPushButton>;
btn->setProperty("slabelRole", "primaryButton"); // 供主题 QSS 按角色选中
btn->setTextTr("保存");                 // 随语言重译
btn->setThemeOverride("color", "#f00"); // 代码覆盖 QSS
QObject::connect(btn, &QPushButton::clicked, btn, [btn]{ btn->triggerOperation(); });
```

`examples/widgets/` 下的 `SButton`/`SLabel`/... 是"业务层如何固化一套自己的具体控件类型"
的参考实现，仅供参考，不是本库对外 API 的一部分。

支持：Qt 5.15.2 / 5.15.10，Windows 7 / Linux，C++17。
