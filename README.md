# standard_lable (slabel)

Qt 5.15 基础控件封装库：统一可切换主题、运行时多语言、双向/单向数据绑定。

## 构建
CMake：`cmake -S . -B build && cmake --build build`
qmake：`qmake standard_lable.pro && make`

## 用法
```cpp
#include "slabel/SButton.h"
#include "slabel/ThemeManager.h"

ThemeManager::instance().registerTheme("dark", "themes/dark.qss");
ThemeManager::instance().setTheme("dark");

SButton* btn = new SButton;
btn->setTextTr("保存");                 // 随语言重译
btn->setThemeOverride("color", "#f00"); // 代码覆盖 QSS
```

支持：Qt 5.15.2 / 5.15.10，Windows 7 / Linux，C++17。
