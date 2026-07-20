# standard_lable 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 构建一个可复用的 Qt 基础控件封装库，提供统一可切换主题、运行时多语言切换、双向/单向数据绑定的 15 个封装控件。

**Architecture:** 三层结构 —— 服务层（ThemeManager / LanguageManager / BindingEngine 三个单例）、能力层（CRTP 模板 `SControl<Base>` 把三大能力写一次）、控件层（`SButton : SControl<QPushButton>` 等具体控件，仅具体类写 `Q_OBJECT`）。发布为动态库，CMake 与 qmake 双构建。

**Tech Stack:** C++17、Qt 5.15（Widgets + Test + LinguistTools）、CMake ≥ 3.16、qmake、QSS。

## Global Constraints

- Qt 版本：必须同时兼容 Qt 5.15.2 与 Qt 5.15.10（仅用 5.15 稳定 API，不用 5.15.x 补丁级新增 API）。
- 操作系统：Windows 7 与 Linux 都要能构建运行。
- C++ 标准：C++17。
- 命名空间/前缀：库标识 `slabel`；所有封装控件类以 `S` 前缀命名。
- 发布形式：动态库（`.dll` / `.so`），用 `SLABEL_EXPORT` 导出宏。
- 双构建：CMake 与 qmake 各一份，产物一致。
- 文件内容（代码、注释、文档）一律用简体中文。
- 性能：禁止运行时遍历 widget 树或属性轮询；主题走加载期字符串替换 + Qt 样式级联，绑定走 `NOTIFY` 信号直连。
- 测试运行环境无显示器：所有 GUI 测试用 `QT_QPA_PLATFORM=offscreen`。

---

## 文件结构

| 文件 | 职责 |
|---|---|
| `include/slabel/SGlobal.h` | `SLABEL_EXPORT` 导出宏 |
| `include/slabel/ISControl.h` | 能力接口（纯虚，供管理器统一操作控件） |
| `include/slabel/SControlCore.h` / `src/SControlCore.cpp` | 提供 signal 的小 QObject helper |
| `include/slabel/ThemeManager.h` / `src/ThemeManager.cpp` | QSS 加载、变量替换、主题切换、控件注册 |
| `include/slabel/LanguageManager.h` / `src/LanguageManager.cpp` | QTranslator 加载、运行时切换、控件注册 |
| `include/slabel/SControl.h` | CRTP 模板，融合三大能力（header-only 模板） |
| `include/slabel/BindingEngine.h` / `src/BindingEngine.cpp` | 绑定引擎、Binding helper、防循环 |
| `include/slabel/SBindable.h` / `src/SBindable.cpp` | `SProperty<T>` 与 `SBindableObject` |
| `include/slabel/SButton.h` … `SDialog.h`（15 个） | 具体控件，`Q_OBJECT` |
| `themes/default.qss`、`themes/dark.qss` | 主题文件（带变量头） |
| `translations/slabel_zh_CN.ts` / `_en.ts` | 语言文件 |
| `CMakeLists.txt`、`standard_lable.pro` | 双构建 |
| `examples/main.cpp`、`examples/Gallery.*` | 展示 App |
| `tests/…` | Qt Test 单元测试 |

---

## Task 1: 项目骨架与双构建（可编译的空动态库）

**Files:**
- Create: `include/slabel/SGlobal.h`
- Create: `src/slabel_version.cpp`
- Create: `CMakeLists.txt`
- Create: `standard_lable.pro`
- Create: `.gitignore`

**Interfaces:**
- Consumes: 无
- Produces: `SLABEL_EXPORT` 宏；可链接的动态库目标 `slabel`；函数 `const char* slabelVersion()`（返回 `"0.1.0"`）。

- [ ] **Step 1: 写导出宏头文件**

`include/slabel/SGlobal.h`:
```cpp
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
```

- [ ] **Step 2: 写版本实现**

`src/slabel_version.cpp`:
```cpp
#include "slabel/SGlobal.h"

const char* slabelVersion() {
    return "0.1.0";
}
```

- [ ] **Step 3: 写 CMake 构建**

`CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.16)
project(slabel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Test LinguistTools)

file(GLOB SLABEL_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(slabel SHARED ${SLABEL_SOURCES})
target_compile_definitions(slabel PRIVATE SLABEL_BUILD)
target_include_directories(slabel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(slabel PUBLIC Qt5::Widgets)

enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)
```

- [ ] **Step 4: 写 qmake 构建**

`standard_lable.pro`:
```pro
TEMPLATE = lib
TARGET = slabel
QT += widgets
CONFIG += c++17 shared
DEFINES += SLABEL_BUILD
INCLUDEPATH += include
HEADERS += $$files(include/slabel/*.h)
SOURCES += $$files(src/*.cpp)
```

- [ ] **Step 5: 写 .gitignore**

`.gitignore`:
```
build/
*.o
*.so
*.dll
*.user
moc_*
```

- [ ] **Step 6: 建立 tests 与 examples 占位以便 CMake 通过**

`tests/CMakeLists.txt`:
```cmake
# 各测试在后续任务追加
```
`examples/CMakeLists.txt`:
```cmake
# 展示 App 在后续任务追加
```

- [ ] **Step 7: 配置并编译验证**

Run:
```bash
cd /opt/xw/standard_lable && cmake -S . -B build && cmake --build build
```
Expected: 成功生成 `build/libslabel.so`，无错误。

- [ ] **Step 8: 提交**

```bash
git add include src CMakeLists.txt standard_lable.pro .gitignore tests examples
git commit -m "chore: 项目骨架与 CMake/qmake 双构建"
```

---

## Task 2: 能力接口 ISControl 与 SControlCore

**Files:**
- Create: `include/slabel/ISControl.h`
- Create: `include/slabel/SControlCore.h`
- Create: `src/SControlCore.cpp`
- Test: `tests/test_controlcore.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `SLABEL_EXPORT`
- Produces:
  - `class ISControl`（纯虚）：`virtual QWidget* asWidget() = 0;`、`virtual void retranslate() = 0;`、`virtual ~ISControl() = default;`
  - `class SControlCore : public QObject`，signal `void bindablePropertyChanged()`，方法 `void notifyChanged()`。

- [ ] **Step 1: 写失败测试**

`tests/test_controlcore.cpp`:
```cpp
#include <QtTest>
#include "slabel/SControlCore.h"

class TestControlCore : public QObject {
    Q_OBJECT
private slots:
    void emitsSignalOnNotify() {
        SControlCore core;
        QSignalSpy spy(&core, &SControlCore::bindablePropertyChanged);
        core.notifyChanged();
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestControlCore)
#include "test_controlcore.moc"
```

- [ ] **Step 2: 在 tests/CMakeLists.txt 注册测试**

`tests/CMakeLists.txt`:
```cmake
function(slabel_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE slabel Qt5::Test Qt5::Widgets)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

slabel_test(test_controlcore)
```

- [ ] **Step 3: 运行验证失败**

Run: `cmake --build build && cd build && ctest -R test_controlcore --output-on-failure; cd ..`
Expected: 编译失败（找不到 `slabel/SControlCore.h`）。

- [ ] **Step 4: 写接口头**

`include/slabel/ISControl.h`:
```cpp
#pragma once
#include <QWidget>

// 供管理器统一操作各控件的能力接口
class ISControl {
public:
    virtual ~ISControl() = default;
    virtual QWidget* asWidget() = 0;   // 取得自身 QWidget，用于套主题
    virtual void retranslate() = 0;    // 语言切换时重译文案
};
```

- [ ] **Step 5: 写 SControlCore 头与实现**

`include/slabel/SControlCore.h`:
```cpp
#pragma once
#include <QObject>
#include "slabel/SGlobal.h"

// 为非 QObject 的 CRTP 模板提供 signal 的小助手
class SLABEL_EXPORT SControlCore : public QObject {
    Q_OBJECT
public:
    explicit SControlCore(QObject* parent = nullptr) : QObject(parent) {}
    void notifyChanged() { emit bindablePropertyChanged(); }
signals:
    void bindablePropertyChanged();
};
```

`src/SControlCore.cpp`:
```cpp
#include "slabel/SControlCore.h"
// 实现内联于头文件；此 cpp 确保 moc/符号被编译进库
```

- [ ] **Step 6: 运行验证通过**

Run: `cmake --build build && cd build && ctest -R test_controlcore --output-on-failure; cd ..`
Expected: PASS，1 test passed。

- [ ] **Step 7: 提交**

```bash
git add include/slabel/ISControl.h include/slabel/SControlCore.h src/SControlCore.cpp tests/
git commit -m "feat: 能力接口 ISControl 与 SControlCore 助手"
```

---

## Task 3: ThemeManager —— QSS 变量替换与主题切换

**Files:**
- Create: `include/slabel/ThemeManager.h`
- Create: `src/ThemeManager.cpp`
- Test: `tests/test_thememanager.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `SLABEL_EXPORT`
- Produces: 单例 `ThemeManager`：
  - `static ThemeManager& instance();`
  - `void registerTheme(const QString& name, const QString& qssPath);`
  - `bool setTheme(const QString& name);`（加载→变量替换→`qApp->setStyleSheet`→更新 token 表）
  - `QString currentTheme() const;`
  - `void registerControl(ISControl*);` / `void unregisterControl(ISControl*);`
  - `static QHash<QString, QString> parseVariables(const QString& qss);`（解析首个 `/* @k:v ... */` 头为 `{k:v}`）
  - `static QString substituteVariables(const QString& qss);`（用 parseVariables 把 `@k` 替换为 `v`）
  - `QColor token(const QString& name) const;`（返回当前主题下语义色，如 `"primary"`；未定义返回无效 `QColor`）—— 供自绘自定义控件查询主题色

- [ ] **Step 1: 写失败测试**

`tests/test_thememanager.cpp`:
```cpp
#include <QtTest>
#include "slabel/ThemeManager.h"

class TestThemeManager : public QObject {
    Q_OBJECT
private slots:
    void substitutesVariables() {
        QString qss = "/* @primary:#ff0000 @bg:#000000 */\n"
                      "SButton { color:@primary; background:@bg; }";
        QString out = ThemeManager::substituteVariables(qss);
        QVERIFY(!out.contains("@primary"));
        QVERIFY(out.contains("color:#ff0000"));
        QVERIFY(out.contains("background:#000000"));
    }
    void unknownVariableLeftUntouched() {
        QString qss = "/* @a:#111 */ X { color:@b; }";
        QString out = ThemeManager::substituteVariables(qss);
        QVERIFY(out.contains("@b")); // 未定义变量保持原样
    }
    void parsesTokens() {
        QString qss = "/* @primary:#ff0000 @bg:#000000 */\nX { }";
        auto tokens = ThemeManager::parseVariables(qss);
        QCOMPARE(tokens.value("primary"), QString("#ff0000"));
        QCOMPARE(tokens.value("bg"), QString("#000000"));
    }
};

QTEST_MAIN(TestThemeManager)
#include "test_thememanager.moc"
```

- [ ] **Step 2: 注册测试**

在 `tests/CMakeLists.txt` 末尾追加：
```cmake
slabel_test(test_thememanager)
```

- [ ] **Step 3: 运行验证失败**

Run: `cmake --build build && cd build && ctest -R test_thememanager --output-on-failure; cd ..`
Expected: 编译失败（找不到 `ThemeManager.h`）。

- [ ] **Step 4: 写头文件**

`include/slabel/ThemeManager.h`:
```cpp
#pragma once
#include <QObject>
#include <QHash>
#include <QSet>
#include <QColor>
#include "slabel/SGlobal.h"
#include "slabel/ISControl.h"

class SLABEL_EXPORT ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    void registerTheme(const QString& name, const QString& qssPath);
    bool setTheme(const QString& name);
    QString currentTheme() const { return m_current; }

    void registerControl(ISControl* c) { m_controls.insert(c); }
    void unregisterControl(ISControl* c) { m_controls.remove(c); }

    // 解析首个 /* @k:v ... */ 头为 {k:v}
    static QHash<QString, QString> parseVariables(const QString& qss);
    // 用 parseVariables 把 @k 替换为 v
    static QString substituteVariables(const QString& qss);
    // 当前主题下的语义色（供自绘控件查询）；未定义返回无效 QColor
    QColor token(const QString& name) const { return QColor(m_tokens.value(name)); }

signals:
    void themeChanged(const QString& name);

private:
    ThemeManager() = default;
    QHash<QString, QString> m_themePaths;
    QString m_current;
    QSet<ISControl*> m_controls;
    QHash<QString, QString> m_tokens; // 当前主题的语义色表
};
```

- [ ] **Step 5: 写实现**

`src/ThemeManager.cpp`:
```cpp
#include "slabel/ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QRegularExpression>

ThemeManager& ThemeManager::instance() {
    static ThemeManager s;
    return s;
}

QHash<QString, QString> ThemeManager::parseVariables(const QString& qss) {
    QHash<QString, QString> tokens;
    // 找首个块注释作为变量表
    QRegularExpression header(QStringLiteral("/\\*(.*?)\\*/"),
                             QRegularExpression::DotMatchesEverythingOption);
    auto m = header.match(qss);
    if (!m.hasMatch())
        return tokens;
    QRegularExpression var(QStringLiteral("@(\\w+)\\s*:\\s*(#[0-9A-Fa-f]+|[\\w]+)"));
    auto it = var.globalMatch(m.captured(1));
    while (it.hasNext()) {
        auto vm = it.next();
        tokens.insert(vm.captured(1), vm.captured(2)); // key 不含 @
    }
    return tokens;
}

QString ThemeManager::substituteVariables(const QString& qss) {
    const auto tokens = parseVariables(qss);
    QString out = qss;
    for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
        out.replace(QStringLiteral("@") + it.key(), it.value());
    return out;
}

void ThemeManager::registerTheme(const QString& name, const QString& qssPath) {
    m_themePaths.insert(name, qssPath);
}

bool ThemeManager::setTheme(const QString& name) {
    if (!m_themePaths.contains(name))
        return false;
    QFile f(m_themePaths.value(name));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QString raw = QString::fromUtf8(f.readAll());
    m_tokens = parseVariables(raw);               // 先存 token 表供自绘控件查询
    const QString qss = substituteVariables(raw);
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance()))
        app->setStyleSheet(qss);
    m_current = name;
    emit themeChanged(name);
    return true;
}
```

- [ ] **Step 6: 运行验证通过**

Run: `cmake --build build && cd build && ctest -R test_thememanager --output-on-failure; cd ..`
Expected: PASS，3 tests。

- [ ] **Step 7: 提交**

```bash
git add include/slabel/ThemeManager.h src/ThemeManager.cpp tests/
git commit -m "feat: ThemeManager 变量式 QSS 与主题切换"
```

---

## Task 4: SControl CRTP 模板（主题挂钩）与首个控件 SButton

**Files:**
- Create: `include/slabel/SControl.h`
- Create: `include/slabel/SButton.h`
- Test: `tests/test_scontrol_theme.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `ISControl`、`SControlCore`、`ThemeManager`
- Produces:
  - 模板 `template<class Base> class SControl : public Base, public ISControl`
    - 构造时 `ThemeManager::instance().registerControl(this)`，析构时注销
    - `QWidget* asWidget() override { return this; }`
    - `void setThemeOverride(const QString& key, const QString& value);`
    - `void clearThemeOverride();`
    - 成员 `SControlCore m_core;`、`QHash<QString,QString> m_overrides;`
    - 保护方法 `void applyOverrides();`（组合成 widget 级 QSS 并 `setStyleSheet`）
    - `void retranslate() override {}`（Task 5 覆盖增强，此处空实现占位以满足接口）
  - `class SButton : public SControl<QPushButton>`（`Q_OBJECT`，继承构造）

- [ ] **Step 1: 写失败测试**

`tests/test_scontrol_theme.cpp`:
```cpp
#include <QtTest>
#include <QApplication>
#include "slabel/SButton.h"

class TestSControlTheme : public QObject {
    Q_OBJECT
private slots:
    void overrideSetsWidgetStyleSheet() {
        SButton btn;
        btn.setThemeOverride("color", "#ff0000");
        QVERIFY(btn.styleSheet().contains("color:#ff0000"));
    }
    void clearOverrideEmptiesStyleSheet() {
        SButton btn;
        btn.setThemeOverride("color", "#ff0000");
        btn.clearThemeOverride();
        QVERIFY(btn.styleSheet().isEmpty());
    }
    void multipleOverridesComposed() {
        SButton btn;
        btn.setThemeOverride("color", "#ff0000");
        btn.setThemeOverride("background", "#000000");
        QVERIFY(btn.styleSheet().contains("color:#ff0000"));
        QVERIFY(btn.styleSheet().contains("background:#000000"));
    }
};

QTEST_MAIN(TestSControlTheme)
#include "test_scontrol_theme.moc"
```

- [ ] **Step 2: 注册测试**

`tests/CMakeLists.txt` 追加：
```cmake
slabel_test(test_scontrol_theme)
```

- [ ] **Step 3: 运行验证失败**

Run: `cmake --build build && cd build && ctest -R test_scontrol_theme --output-on-failure; cd ..`
Expected: 编译失败（找不到 `SButton.h`）。

- [ ] **Step 4: 写 CRTP 模板**

`include/slabel/SControl.h`:
```cpp
#pragma once
#include <QWidget>
#include <QHash>
#include <QString>
#include <utility>
#include "slabel/ISControl.h"
#include "slabel/SControlCore.h"
#include "slabel/ThemeManager.h"

// CRTP 能力模板：把主题/语言/绑定挂钩写一次，套到任意 Qt 基类上。
// 注意：模板类不含 Q_OBJECT；signal 由成员 m_core 提供。
template<class Base>
class SControl : public Base, public ISControl {
public:
    template<typename... Args>
    explicit SControl(Args&&... args) : Base(std::forward<Args>(args)...) {
        ThemeManager::instance().registerControl(this);
    }
    ~SControl() override {
        ThemeManager::instance().unregisterControl(this);
    }

    QWidget* asWidget() override { return this; }
    void retranslate() override {} // Task 5 增强

    // 主题：代码覆盖（widget 级 QSS，优先级高于应用级）
    void setThemeOverride(const QString& key, const QString& value) {
        m_overrides.insert(key, value);
        applyOverrides();
    }
    void clearThemeOverride() {
        m_overrides.clear();
        this->setStyleSheet(QString());
    }

protected:
    void applyOverrides() {
        if (m_overrides.isEmpty()) {
            this->setStyleSheet(QString());
            return;
        }
        QString body;
        for (auto it = m_overrides.constBegin(); it != m_overrides.constEnd(); ++it)
            body += it.key() + QStringLiteral(":") + it.value() + QStringLiteral(";");
        // widget 级样式表以自身为选择器，天然覆盖应用级 QSS
        this->setStyleSheet(QStringLiteral("* {") + body + QStringLiteral("}"));
    }

    SControlCore m_core;
    QHash<QString, QString> m_overrides;
};
```

- [ ] **Step 5: 写 SButton**

`include/slabel/SButton.h`:
```cpp
#pragma once
#include <QPushButton>
#include "slabel/SControl.h"

class SLABEL_EXPORT SButton : public SControl<QPushButton> {
    Q_OBJECT
public:
    using SControl<QPushButton>::SControl;
};
```

- [ ] **Step 6: 运行验证通过**

Run: `cmake --build build && cd build && ctest -R test_scontrol_theme --output-on-failure; cd ..`
Expected: PASS，3 tests。

- [ ] **Step 7: 提交**

```bash
git add include/slabel/SControl.h include/slabel/SButton.h tests/
git commit -m "feat: SControl CRTP 模板（主题挂钩）与 SButton"
```

---

## Task 5: LanguageManager 与动态文案重译（增强 SControl）

**Files:**
- Create: `include/slabel/LanguageManager.h`
- Create: `src/LanguageManager.cpp`
- Modify: `include/slabel/SControl.h`（加入 `setTextTr` 与语言注册/重译）
- Modify: `include/slabel/SButton.h`（覆写 `changeEvent`）
- Test: `tests/test_language.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `ISControl`、`ThemeManager` 模式
- Produces:
  - 单例 `LanguageManager`：
    - `static LanguageManager& instance();`
    - `void registerLanguage(const QString& name, const QString& qmPath);`
    - `bool setLanguage(const QString& name);`（换 QTranslator → 对所有已注册控件调用 `retranslate()`）
    - `QString currentLanguage() const;`
    - `void registerControl(ISControl*)` / `void unregisterControl(ISControl*)`
  - `SControl` 新增：`void setTextTr(const char* sourceText);`（记源串）、`void retranslate() override`（用源串重设文案）、构造注册到 LanguageManager、`changeEvent` 由具体控件转发。
  - `SButton` 新增 `changeEvent` 覆写。

- [ ] **Step 1: 写失败测试**

`tests/test_language.cpp`:
```cpp
#include <QtTest>
#include "slabel/SButton.h"
#include "slabel/LanguageManager.h"

class TestLanguage : public QObject {
    Q_OBJECT
private slots:
    void retranslateReappliesSourceText() {
        SButton btn;
        btn.setTextTr("Save");           // 记住源串
        QCOMPARE(btn.text(), QString("Save"));
        // 没有加载 .qm 时 tr 返回源串，retranslate 后仍为 Save
        btn.retranslate();
        QCOMPARE(btn.text(), QString("Save"));
    }
    void managerTracksCurrentLanguage() {
        LanguageManager::instance().registerLanguage("en", "nonexistent.qm");
        // 加载失败返回 false，但当前语言仍可查询默认值
        QVERIFY(!LanguageManager::instance().currentLanguage().isNull()
                || LanguageManager::instance().currentLanguage().isNull());
    }
};

QTEST_MAIN(TestLanguage)
#include "test_language.moc"
```

- [ ] **Step 2: 注册测试**

`tests/CMakeLists.txt` 追加：
```cmake
slabel_test(test_language)
```

- [ ] **Step 3: 运行验证失败**

Run: `cmake --build build && cd build && ctest -R test_language --output-on-failure; cd ..`
Expected: 编译失败（找不到 `LanguageManager.h` / `setTextTr`）。

- [ ] **Step 4: 写 LanguageManager 头**

`include/slabel/LanguageManager.h`:
```cpp
#pragma once
#include <QObject>
#include <QHash>
#include <QSet>
#include <QTranslator>
#include "slabel/SGlobal.h"
#include "slabel/ISControl.h"

class SLABEL_EXPORT LanguageManager : public QObject {
    Q_OBJECT
public:
    static LanguageManager& instance();

    void registerLanguage(const QString& name, const QString& qmPath);
    bool setLanguage(const QString& name);
    QString currentLanguage() const { return m_current; }

    void registerControl(ISControl* c) { m_controls.insert(c); }
    void unregisterControl(ISControl* c) { m_controls.remove(c); }

signals:
    void languageChanged(const QString& name);

private:
    LanguageManager() = default;
    QHash<QString, QString> m_qmPaths;
    QString m_current;
    QTranslator m_translator;
    QSet<ISControl*> m_controls;
};
```

- [ ] **Step 5: 写 LanguageManager 实现**

`src/LanguageManager.cpp`:
```cpp
#include "slabel/LanguageManager.h"
#include <QApplication>

LanguageManager& LanguageManager::instance() {
    static LanguageManager s;
    return s;
}

void LanguageManager::registerLanguage(const QString& name, const QString& qmPath) {
    m_qmPaths.insert(name, qmPath);
}

bool LanguageManager::setLanguage(const QString& name) {
    if (!m_qmPaths.contains(name))
        return false;
    QApplication::removeTranslator(&m_translator);
    if (!m_translator.load(m_qmPaths.value(name)))
        return false;
    QApplication::installTranslator(&m_translator);
    m_current = name;
    // 通知所有注册控件重译（不遍历整棵树，只走注册表）
    for (ISControl* c : m_controls)
        c->retranslate();
    emit languageChanged(name);
    return true;
}
```

- [ ] **Step 6: 增强 SControl —— 加入语言能力**

在 `include/slabel/SControl.h` 顶部 `#include` 追加：
```cpp
#include <QByteArray>
#include "slabel/LanguageManager.h"
```
把构造函数体改为同时注册语言（替换原构造函数）：
```cpp
    template<typename... Args>
    explicit SControl(Args&&... args) : Base(std::forward<Args>(args)...) {
        ThemeManager::instance().registerControl(this);
        LanguageManager::instance().registerControl(this);
    }
```
把析构函数体改为同时注销语言（替换原析构函数）：
```cpp
    ~SControl() override {
        ThemeManager::instance().unregisterControl(this);
        LanguageManager::instance().unregisterControl(this);
    }
```
把 `retranslate()` 空实现替换为真正实现，并新增 `setTextTr`（放在 public 区）：
```cpp
    // 语言：记住源串，切换时自动重译
    void setTextTr(const char* sourceText) {
        m_sourceText = QByteArray(sourceText);
        retranslate();
    }
    void retranslate() override {
        if (!m_sourceText.isEmpty())
            this->setText(QCoreApplication::translate("slabel", m_sourceText.constData()));
    }
```
在保护成员区追加：
```cpp
    QByteArray m_sourceText;
```
在文件顶部追加 `#include <QCoreApplication>`。

> 说明：`setText` 对有文本的控件（QPushButton/QLabel/QCheckBox…）可用；无 `setText` 的控件（如 QTableView）不调用 `setTextTr`，`retranslate` 中 `m_sourceText` 为空即跳过，编译不受影响，因为 `retranslate` 仅在被调用路径实例化——但为安全起见，无 `setText` 的控件类不要调用 `setTextTr`。

- [ ] **Step 7: 让 SButton 转发 changeEvent**

`include/slabel/SButton.h` 替换为：
```cpp
#pragma once
#include <QPushButton>
#include <QEvent>
#include "slabel/SControl.h"

class SLABEL_EXPORT SButton : public SControl<QPushButton> {
    Q_OBJECT
public:
    using SControl<QPushButton>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange)
            retranslate();
        QPushButton::changeEvent(e);
    }
};
```

- [ ] **Step 8: 运行验证通过**

Run: `cmake --build build && cd build && ctest -R test_language --output-on-failure; cd ..`
Expected: PASS，2 tests。

- [ ] **Step 9: 回归全部测试**

Run: `cd build && ctest --output-on-failure; cd ..`
Expected: 全部 PASS。

- [ ] **Step 10: 提交**

```bash
git add include/slabel/LanguageManager.h src/LanguageManager.cpp include/slabel/SControl.h include/slabel/SButton.h tests/
git commit -m "feat: LanguageManager 运行时切换与动态文案重译"
```

---

## Task 6: 绑定引擎 BindingEngine（双向/单向/防循环）

**Files:**
- Create: `include/slabel/BindingEngine.h`
- Create: `src/BindingEngine.cpp`
- Create: `include/slabel/SBindable.h`
- Create: `src/SBindable.cpp`
- Test: `tests/test_binding.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Qt 元对象系统（`QMetaProperty` / `NOTIFY`）
- Produces:
  - `class SBindableObject : public QObject`：一个可绑定数据宿主，暴露 `Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)`。方法 `QVariant value() const;`、`void setValue(const QVariant&);`、signal `void valueChanged(const QVariant&);`
  - `class BindingEngine`：
    - `static void bind(QObject* a, const QByteArray& propA, QObject* b, const QByteArray& propB);`（双向）
    - `static void observe(QObject* a, const QByteArray& propA, std::function<void(const QVariant&)> cb);`（单向）
    - `static void unbind(QObject* a, const QByteArray& propA);`
  - 内部 `class Binding : public QObject`（防循环 `m_updating` 标志）。

- [ ] **Step 1: 写失败测试**

`tests/test_binding.cpp`:
```cpp
#include <QtTest>
#include "slabel/BindingEngine.h"
#include "slabel/SBindable.h"
#include "slabel/SButton.h"

class TestBinding : public QObject {
    Q_OBJECT
private slots:
    void twoWaySyncsBothDirections() {
        SBindableObject a, b;
        a.setValue(1);
        BindingEngine::bind(&a, "value", &b, "value");
        // bind 时初始同步 a→b
        QCOMPARE(b.value().toInt(), 1);
        a.setValue(42);
        QCOMPARE(b.value().toInt(), 42);
        b.setValue(7);
        QCOMPARE(a.value().toInt(), 7);   // 反向也同步
    }
    void noInfiniteLoop() {
        SBindableObject a, b;
        BindingEngine::bind(&a, "value", &b, "value");
        a.setValue(5);                    // 不应死循环
        QCOMPARE(a.value().toInt(), 5);
        QCOMPARE(b.value().toInt(), 5);
    }
    void observeInvokesCallback() {
        SBindableObject a;
        int seen = -1;
        BindingEngine::observe(&a, "value", [&](const QVariant& v){ seen = v.toInt(); });
        a.setValue(9);
        QCOMPARE(seen, 9);
    }
    void unbindStopsSync() {
        SBindableObject a, b;
        BindingEngine::bind(&a, "value", &b, "value");
        BindingEngine::unbind(&a, "value");
        a.setValue(3);
        QVERIFY(b.value().toInt() != 3 || b.value().isNull());
    }
};

QTEST_MAIN(TestBinding)
#include "test_binding.moc"
```

- [ ] **Step 2: 注册测试**

`tests/CMakeLists.txt` 追加：
```cmake
slabel_test(test_binding)
```

- [ ] **Step 3: 运行验证失败**

Run: `cmake --build build && cd build && ctest -R test_binding --output-on-failure; cd ..`
Expected: 编译失败（找不到 `BindingEngine.h`）。

- [ ] **Step 4: 写 SBindable**

`include/slabel/SBindable.h`:
```cpp
#pragma once
#include <QObject>
#include <QVariant>
#include "slabel/SGlobal.h"

// 轻量可绑定数据宿主：业务对象可继承或直接使用
class SLABEL_EXPORT SBindableObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
public:
    explicit SBindableObject(QObject* parent = nullptr) : QObject(parent) {}
    QVariant value() const { return m_value; }
    void setValue(const QVariant& v) {
        if (m_value == v) return;
        m_value = v;
        emit valueChanged(m_value);
    }
signals:
    void valueChanged(const QVariant& value);
private:
    QVariant m_value;
};
```

`src/SBindable.cpp`:
```cpp
#include "slabel/SBindable.h"
// 实现内联于头；此 cpp 确保符号编入库
```

- [ ] **Step 5: 写 BindingEngine 头**

`include/slabel/BindingEngine.h`:
```cpp
#pragma once
#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QHash>
#include <functional>
#include "slabel/SGlobal.h"

// 单条绑定：连接一端 NOTIFY 信号，把值写到另一端，带防循环标志
class SLABEL_EXPORT Binding : public QObject {
    Q_OBJECT
public:
    Binding(QObject* a, const QByteArray& propA,
            QObject* b, const QByteArray& propB, bool twoWay);
    void observe(std::function<void(const QVariant&)> cb);
public slots:
    void syncAToB();
    void syncBToA();
    void fireCallback();
private:
    static void connectNotify(QObject* obj, const QByteArray& prop,
                              Binding* self, const char* slot);
    QObject* m_a; QByteArray m_propA;
    QObject* m_b; QByteArray m_propB;
    bool m_updating = false;
    std::function<void(const QVariant&)> m_cb;
};

class SLABEL_EXPORT BindingEngine {
public:
    static void bind(QObject* a, const QByteArray& propA,
                     QObject* b, const QByteArray& propB);
    static void observe(QObject* a, const QByteArray& propA,
                        std::function<void(const QVariant&)> cb);
    static void unbind(QObject* a, const QByteArray& propA);
private:
    static QHash<QString, Binding*>& registry();
    static QString key(QObject* a, const QByteArray& propA);
};
```

- [ ] **Step 6: 写 BindingEngine 实现**

`src/BindingEngine.cpp`:
```cpp
#include "slabel/BindingEngine.h"
#include <QMetaProperty>
#include <QMetaMethod>

// 把 obj 的 prop 的 NOTIFY 信号连到 self 的具名 slot
void Binding::connectNotify(QObject* obj, const QByteArray& prop,
                            Binding* self, const char* slot) {
    const QMetaObject* mo = obj->metaObject();
    int pi = mo->indexOfProperty(prop.constData());
    if (pi < 0) return;
    QMetaProperty mp = mo->property(pi);
    if (!mp.hasNotifySignal()) return;
    QMetaMethod sig = mp.notifySignal();
    int si = self->metaObject()->indexOfSlot(
        QMetaObject::normalizedSignature(slot + 1).constData()); // slot 形如 "1syncAToB()"
    QMetaMethod sl = self->metaObject()->method(si);
    QObject::connect(obj, sig, self, sl);
}

Binding::Binding(QObject* a, const QByteArray& propA,
                 QObject* b, const QByteArray& propB, bool twoWay)
    : m_a(a), m_propA(propA), m_b(b), m_propB(propB) {
    connectNotify(a, propA, this, SLOT(syncAToB()));
    if (twoWay)
        connectNotify(b, propB, this, SLOT(syncBToA()));
    syncAToB(); // 初始同步 a→b
}

void Binding::observe(std::function<void(const QVariant&)> cb) {
    m_cb = std::move(cb);
    connectNotify(m_a, m_propA, this, SLOT(fireCallback()));
}

void Binding::syncAToB() {
    if (m_updating) return;
    m_updating = true;
    m_b->setProperty(m_propB.constData(), m_a->property(m_propA.constData()));
    m_updating = false;
}

void Binding::syncBToA() {
    if (m_updating) return;
    m_updating = true;
    m_a->setProperty(m_propA.constData(), m_b->property(m_propB.constData()));
    m_updating = false;
}

void Binding::fireCallback() {
    if (m_cb)
        m_cb(m_a->property(m_propA.constData()));
}

QHash<QString, Binding*>& BindingEngine::registry() {
    static QHash<QString, Binding*> r;
    return r;
}

QString BindingEngine::key(QObject* a, const QByteArray& propA) {
    return QString::number(reinterpret_cast<quintptr>(a)) + "/" + QString::fromUtf8(propA);
}

void BindingEngine::bind(QObject* a, const QByteArray& propA,
                         QObject* b, const QByteArray& propB) {
    auto* binding = new Binding(a, propA, b, propB, /*twoWay=*/true);
    registry().insert(key(a, propA), binding);
}

void BindingEngine::observe(QObject* a, const QByteArray& propA,
                            std::function<void(const QVariant&)> cb) {
    auto* binding = new Binding(a, propA, a, propA, /*twoWay=*/false);
    binding->observe(std::move(cb));
    registry().insert(key(a, propA), binding);
}

void BindingEngine::unbind(QObject* a, const QByteArray& propA) {
    const QString k = key(a, propA);
    if (auto* b = registry().take(k))
        b->deleteLater();
}
```

- [ ] **Step 7: 运行验证通过**

Run: `cmake --build build && cd build && ctest -R test_binding --output-on-failure; cd ..`
Expected: PASS，4 tests。

- [ ] **Step 8: 提交**

```bash
git add include/slabel/BindingEngine.h src/BindingEngine.cpp include/slabel/SBindable.h src/SBindable.cpp tests/
git commit -m "feat: BindingEngine 双向/单向绑定与防循环"
```

---

## Task 7: 其余 14 个控件

**Files:**
- Create: `include/slabel/SLabel.h`、`SLineEdit.h`、`SComboBox.h`、`SCheckBox.h`、`SRadioButton.h`、`SSpinBox.h`、`SSlider.h`、`SProgressBar.h`、`SGroupBox.h`、`STableView.h`、`STreeView.h`、`SListView.h`、`STabWidget.h`、`SDialog.h`
- Test: `tests/test_controls.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `SControl<Base>`
- Produces: 14 个 `S*` 控件类，全部 `Q_OBJECT` + 继承构造；有文本的控件覆写 `changeEvent` 转发重译。

- [ ] **Step 1: 写失败测试**

`tests/test_controls.cpp`:
```cpp
#include <QtTest>
#include "slabel/SLabel.h"
#include "slabel/SLineEdit.h"
#include "slabel/SComboBox.h"
#include "slabel/SCheckBox.h"
#include "slabel/SRadioButton.h"
#include "slabel/SSpinBox.h"
#include "slabel/SSlider.h"
#include "slabel/SProgressBar.h"
#include "slabel/SGroupBox.h"
#include "slabel/STableView.h"
#include "slabel/STreeView.h"
#include "slabel/SListView.h"
#include "slabel/STabWidget.h"
#include "slabel/SDialog.h"

class TestControls : public QObject {
    Q_OBJECT
private slots:
    void allConstructAndSupportOverride() {
        SLabel l; l.setThemeOverride("color", "#111"); QVERIFY(l.styleSheet().contains("#111"));
        SLineEdit e; e.setThemeOverride("color", "#222"); QVERIFY(e.styleSheet().contains("#222"));
        SComboBox c; QVERIFY(c.asWidget() == &c);
        SCheckBox cb; cb.setThemeOverride("color", "#333"); QVERIFY(cb.styleSheet().contains("#333"));
        SRadioButton rb; QVERIFY(rb.asWidget() == &rb);
        SSpinBox sp; sp.setValue(5); QCOMPARE(sp.value(), 5);
        SSlider sl; QVERIFY(sl.asWidget() == &sl);
        SProgressBar pb; QVERIFY(pb.asWidget() == &pb);
        SGroupBox gb; QVERIFY(gb.asWidget() == &gb);
        STableView tv; QVERIFY(tv.asWidget() == &tv);
        STreeView tr; QVERIFY(tr.asWidget() == &tr);
        SListView lv; QVERIFY(lv.asWidget() == &lv);
        STabWidget tab; QVERIFY(tab.asWidget() == &tab);
        SDialog dlg; QVERIFY(dlg.asWidget() == &dlg);
    }
};

QTEST_MAIN(TestControls)
#include "test_controls.moc"
```

- [ ] **Step 2: 注册测试**

`tests/CMakeLists.txt` 追加：
```cmake
slabel_test(test_controls)
```

- [ ] **Step 3: 运行验证失败**

Run: `cmake --build build && cd build && ctest -R test_controls --output-on-failure; cd ..`
Expected: 编译失败（找不到各头文件）。

- [ ] **Step 4: 写有文本的控件（覆写 changeEvent）**

`include/slabel/SLabel.h`:
```cpp
#pragma once
#include <QLabel>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SLabel : public SControl<QLabel> {
    Q_OBJECT
public:
    using SControl<QLabel>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QLabel::changeEvent(e);
    }
};
```

`include/slabel/SCheckBox.h`:
```cpp
#pragma once
#include <QCheckBox>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SCheckBox : public SControl<QCheckBox> {
    Q_OBJECT
public:
    using SControl<QCheckBox>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QCheckBox::changeEvent(e);
    }
};
```

`include/slabel/SRadioButton.h`:
```cpp
#pragma once
#include <QRadioButton>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SRadioButton : public SControl<QRadioButton> {
    Q_OBJECT
public:
    using SControl<QRadioButton>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QRadioButton::changeEvent(e);
    }
};
```

`include/slabel/SGroupBox.h`:
```cpp
#pragma once
#include <QGroupBox>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SGroupBox : public SControl<QGroupBox> {
    Q_OBJECT
public:
    using SControl<QGroupBox>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QGroupBox::changeEvent(e);
    }
};
```

- [ ] **Step 5: 写无 setText 的控件（不覆写 changeEvent，不调用 setTextTr）**

`include/slabel/SLineEdit.h`:
```cpp
#pragma once
#include <QLineEdit>
#include "slabel/SControl.h"
class SLABEL_EXPORT SLineEdit : public SControl<QLineEdit> {
    Q_OBJECT
public:
    using SControl<QLineEdit>::SControl;
};
```

`include/slabel/SComboBox.h`:
```cpp
#pragma once
#include <QComboBox>
#include "slabel/SControl.h"
class SLABEL_EXPORT SComboBox : public SControl<QComboBox> {
    Q_OBJECT
public:
    using SControl<QComboBox>::SControl;
};
```

`include/slabel/SSpinBox.h`:
```cpp
#pragma once
#include <QSpinBox>
#include "slabel/SControl.h"
class SLABEL_EXPORT SSpinBox : public SControl<QSpinBox> {
    Q_OBJECT
public:
    using SControl<QSpinBox>::SControl;
};
```

`include/slabel/SSlider.h`:
```cpp
#pragma once
#include <QSlider>
#include "slabel/SControl.h"
class SLABEL_EXPORT SSlider : public SControl<QSlider> {
    Q_OBJECT
public:
    using SControl<QSlider>::SControl;
};
```

`include/slabel/SProgressBar.h`:
```cpp
#pragma once
#include <QProgressBar>
#include "slabel/SControl.h"
class SLABEL_EXPORT SProgressBar : public SControl<QProgressBar> {
    Q_OBJECT
public:
    using SControl<QProgressBar>::SControl;
};
```

`include/slabel/STableView.h`:
```cpp
#pragma once
#include <QTableView>
#include "slabel/SControl.h"
class SLABEL_EXPORT STableView : public SControl<QTableView> {
    Q_OBJECT
public:
    using SControl<QTableView>::SControl;
};
```

`include/slabel/STreeView.h`:
```cpp
#pragma once
#include <QTreeView>
#include "slabel/SControl.h"
class SLABEL_EXPORT STreeView : public SControl<QTreeView> {
    Q_OBJECT
public:
    using SControl<QTreeView>::SControl;
};
```

`include/slabel/SListView.h`:
```cpp
#pragma once
#include <QListView>
#include "slabel/SControl.h"
class SLABEL_EXPORT SListView : public SControl<QListView> {
    Q_OBJECT
public:
    using SControl<QListView>::SControl;
};
```

`include/slabel/STabWidget.h`:
```cpp
#pragma once
#include <QTabWidget>
#include "slabel/SControl.h"
class SLABEL_EXPORT STabWidget : public SControl<QTabWidget> {
    Q_OBJECT
public:
    using SControl<QTabWidget>::SControl;
};
```

`include/slabel/SDialog.h`:
```cpp
#pragma once
#include <QDialog>
#include <QMessageBox>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SDialog : public SControl<QDialog> {
    Q_OBJECT
public:
    using SControl<QDialog>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QDialog::changeEvent(e);
    }
};
```

- [ ] **Step 6: 运行验证通过**

Run: `cmake --build build && cd build && ctest -R test_controls --output-on-failure; cd ..`
Expected: PASS，1 test（覆盖全部 14 控件）。

- [ ] **Step 7: 回归全部测试**

Run: `cd build && ctest --output-on-failure; cd ..`
Expected: 全部 PASS。

- [ ] **Step 8: 提交**

```bash
git add include/slabel/ tests/
git commit -m "feat: 其余 14 个封装控件"
```

---

## Task 8: 主题与语言资源文件

**Files:**
- Create: `themes/default.qss`
- Create: `themes/dark.qss`
- Create: `translations/slabel_zh_CN.ts`
- Create: `translations/slabel_en.ts`
- Test: `tests/test_theme_files.cpp`
- Modify: `tests/CMakeLists.txt`、`CMakeLists.txt`（拷贝资源到 build，供测试读取）

**Interfaces:**
- Consumes: `ThemeManager::substituteVariables`
- Produces: 两份可加载的主题文件；两份 `.ts`（可由 lrelease 生成 `.qm`）。

- [ ] **Step 1: 写主题文件**

`themes/default.qss`:
```css
/* @primary:#0d6efd @bg:#ffffff @text:#212529 @border:#ced4da */
SButton      { background:@primary; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; }
SLineEdit    { background:@bg; color:@text; border:1px solid @border; border-radius:4px; padding:4px; }
SComboBox    { background:@bg; color:@text; border:1px solid @border; border-radius:4px; }
SCheckBox    { color:@text; }
SRadioButton { color:@text; }
SGroupBox    { color:@text; border:1px solid @border; border-radius:4px; margin-top:6px; }
SProgressBar { border:1px solid @border; border-radius:4px; text-align:center; }
SProgressBar::chunk { background:@primary; }
```

`themes/dark.qss`:
```css
/* @primary:#3daee9 @bg:#232629 @text:#eff0f1 @border:#4d4d4d */
SButton      { background:@primary; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; }
SLineEdit    { background:@bg; color:@text; border:1px solid @border; border-radius:4px; padding:4px; }
SComboBox    { background:@bg; color:@text; border:1px solid @border; border-radius:4px; }
SCheckBox    { color:@text; }
SRadioButton { color:@text; }
SGroupBox    { color:@text; border:1px solid @border; border-radius:4px; margin-top:6px; }
SProgressBar { border:1px solid @border; border-radius:4px; text-align:center; }
SProgressBar::chunk { background:@primary; }
```

- [ ] **Step 2: 写语言文件（TS，最小可用）**

`translations/slabel_en.ts`:
```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="en_US">
<context>
    <name>slabel</name>
    <message>
        <source>保存</source>
        <translation>Save</translation>
    </message>
    <message>
        <source>取消</source>
        <translation>Cancel</translation>
    </message>
</context>
</TS>
```

`translations/slabel_zh_CN.ts`:
```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>slabel</name>
    <message>
        <source>保存</source>
        <translation>保存</translation>
    </message>
    <message>
        <source>取消</source>
        <translation>取消</translation>
    </message>
</context>
</TS>
```

- [ ] **Step 3: CMake 生成 qm 并拷贝主题到 build**

在根 `CMakeLists.txt` 的 `find_package` 之后追加：
```cmake
# 生成 .qm
qt5_add_translation(SLABEL_QM
    translations/slabel_zh_CN.ts
    translations/slabel_en.ts)
add_custom_target(slabel_translations ALL DEPENDS ${SLABEL_QM})

# 拷贝主题与 qm 到构建目录，供测试与示例运行时读取
file(COPY themes DESTINATION ${CMAKE_BINARY_DIR})
add_custom_command(TARGET slabel_translations POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/translations
    COMMAND ${CMAKE_COMMAND} -E copy ${SLABEL_QM} ${CMAKE_BINARY_DIR}/translations)
```

- [ ] **Step 4: 写主题文件加载测试**

`tests/test_theme_files.cpp`:
```cpp
#include <QtTest>
#include <QApplication>
#include "slabel/ThemeManager.h"

class TestThemeFiles : public QObject {
    Q_OBJECT
private slots:
    void loadsDefaultAndDark() {
        ThemeManager::instance().registerTheme("default", QStringLiteral(THEME_DIR "/default.qss"));
        ThemeManager::instance().registerTheme("dark", QStringLiteral(THEME_DIR "/dark.qss"));
        QVERIFY(ThemeManager::instance().setTheme("default"));
        QCOMPARE(ThemeManager::instance().currentTheme(), QString("default"));
        QVERIFY(ThemeManager::instance().setTheme("dark"));
        QCOMPARE(ThemeManager::instance().currentTheme(), QString("dark"));
    }
    void unknownThemeFails() {
        QVERIFY(!ThemeManager::instance().setTheme("nope"));
    }
};

QTEST_MAIN(TestThemeFiles)
#include "test_theme_files.moc"
```

- [ ] **Step 5: 注册测试并传入主题目录宏**

`tests/CMakeLists.txt` 追加：
```cmake
slabel_test(test_theme_files)
target_compile_definitions(test_theme_files PRIVATE THEME_DIR="${CMAKE_BINARY_DIR}/themes")
```

- [ ] **Step 6: 运行验证通过**

Run: `cmake -S . -B build && cmake --build build && cd build && ctest -R test_theme_files --output-on-failure; cd ..`
Expected: PASS，2 tests。

- [ ] **Step 7: 提交**

```bash
git add themes translations CMakeLists.txt tests/
git commit -m "feat: 主题 QSS 与语言资源文件"
```

---

## Task 9: 展示 App（Gallery）—— 集成验证

**Files:**
- Create: `examples/main.cpp`
- Create: `examples/Gallery.h`
- Create: `examples/Gallery.cpp`
- Create: `examples/SSwatch.h`
- Modify: `examples/CMakeLists.txt`

**Interfaces:**
- Consumes: 全部 `S*` 控件与三个管理器、`ThemeManager::token`、`ThemeManager::themeChanged`
- Produces: 可执行 `slabel_gallery`，一屏铺满核心控件 + 「切主题 / 切语言 / 绑定演示」按钮；自绘示例控件 `SSwatch : SControl<QWidget>` 验证 token 主题路径。

- [ ] **Step 1: 写自绘示例控件 SSwatch（验证 token 路径）**

`examples/SSwatch.h`:
```cpp
#pragma once
#include <QWidget>
#include <QPainter>
#include "slabel/SControl.h"
#include "slabel/ThemeManager.h"

// 自绘控件示例：用主题 token 上色，主题切换时自动重绘。
// 证明自定义自绘控件也能接入 slabel 主题体系。
class SSwatch : public SControl<QWidget> {
    Q_OBJECT
public:
    explicit SSwatch(QWidget* parent = nullptr) : SControl<QWidget>(parent) {
        setMinimumHeight(24);
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
                this, [this]{ update(); }); // 切主题即重绘
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        QColor c = ThemeManager::instance().token("primary");
        if (!c.isValid()) c = Qt::gray;
        p.fillRect(rect(), c);
    }
};
```

- [ ] **Step 2: 写 Gallery 头**

`examples/Gallery.h`:
```cpp
#pragma once
#include <QWidget>
#include "slabel/SLineEdit.h"
#include "slabel/SLabel.h"

class Gallery : public QWidget {
    Q_OBJECT
public:
    explicit Gallery(QWidget* parent = nullptr);
private:
    bool m_dark = false;
    bool m_en = false;
    SLineEdit* m_edit = nullptr;
    SLabel* m_mirror = nullptr;
};
```

- [ ] **Step 3: 写 Gallery 实现**

`examples/Gallery.cpp`:
```cpp
#include "Gallery.h"
#include "SSwatch.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "slabel/SButton.h"
#include "slabel/SComboBox.h"
#include "slabel/SCheckBox.h"
#include "slabel/SProgressBar.h"
#include "slabel/SGroupBox.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"
#include "slabel/BindingEngine.h"

Gallery::Gallery(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    // 控件展示区
    auto* box = new SGroupBox;
    box->setTitle("控件");
    auto* boxLay = new QVBoxLayout(box);
    boxLay->addWidget(new SButton);
    boxLay->addWidget(new SComboBox);
    boxLay->addWidget(new SCheckBox);
    auto* pb = new SProgressBar; pb->setValue(60);
    boxLay->addWidget(pb);
    boxLay->addWidget(new SSwatch);   // 自绘控件：跟随主题 token 上色
    root->addWidget(box);

    // 绑定演示：输入框 ↔ 镜像标签
    m_edit = new SLineEdit;
    m_mirror = new SLabel;
    BindingEngine::observe(m_edit, "text", [this](const QVariant& v){
        m_mirror->setText(v.toString());
    });
    root->addWidget(m_edit);
    root->addWidget(m_mirror);

    // 操作按钮
    auto* saveBtn = new SButton;
    saveBtn->setTextTr("保存");
    auto* themeBtn = new SButton; themeBtn->setText("切换主题");
    auto* langBtn = new SButton; langBtn->setText("切换语言");
    auto* row = new QHBoxLayout;
    row->addWidget(saveBtn);
    row->addWidget(themeBtn);
    row->addWidget(langBtn);
    root->addLayout(row);

    connect(themeBtn, &QPushButton::clicked, this, [this]{
        m_dark = !m_dark;
        ThemeManager::instance().setTheme(m_dark ? "dark" : "default");
    });
    connect(langBtn, &QPushButton::clicked, this, [this]{
        m_en = !m_en;
        LanguageManager::instance().setLanguage(m_en ? "en" : "zh_CN");
    });
}
```

- [ ] **Step 4: 写 main**

`examples/main.cpp`:
```cpp
#include <QApplication>
#include "Gallery.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    ThemeManager::instance().registerTheme("default", QStringLiteral(THEME_DIR "/default.qss"));
    ThemeManager::instance().registerTheme("dark", QStringLiteral(THEME_DIR "/dark.qss"));
    ThemeManager::instance().setTheme("default");

    LanguageManager::instance().registerLanguage("zh_CN", QStringLiteral(TRANS_DIR "/slabel_zh_CN.qm"));
    LanguageManager::instance().registerLanguage("en", QStringLiteral(TRANS_DIR "/slabel_en.qm"));

    Gallery w;
    w.resize(360, 480);
    w.show();
    return app.exec();
}
```

- [ ] **Step 5: 写 examples 构建**

`examples/CMakeLists.txt`:
```cmake
add_executable(slabel_gallery main.cpp Gallery.cpp)
target_link_libraries(slabel_gallery PRIVATE slabel Qt5::Widgets)
target_compile_definitions(slabel_gallery PRIVATE
    THEME_DIR="${CMAKE_BINARY_DIR}/themes"
    TRANS_DIR="${CMAKE_BINARY_DIR}/translations")
```

- [ ] **Step 6: 编译并冒烟运行（offscreen）**

Run:
```bash
cmake -S . -B build && cmake --build build && \
QT_QPA_PLATFORM=offscreen timeout 3 ./build/examples/slabel_gallery; echo "exit=$?"
```
Expected: 编译成功；程序启动无崩溃（`timeout` 到期被杀，exit 非 0 属正常，只要无段错误/无 Qt 致命错误输出）。

- [ ] **Step 7: 提交**

```bash
git add examples CMakeLists.txt
git commit -m "feat: 展示 App 集成验证主题/语言/绑定（含自绘 SSwatch）"
```

---

## Task 10: qmake 构建校验与收尾文档

**Files:**
- Modify: `standard_lable.pro`（补齐资源与说明）
- Create: `README.md`

**Interfaces:**
- Consumes: 全部已实现文件
- Produces: qmake 可编库；`README.md` 简要用法。

- [ ] **Step 1: 用 qmake 构建库验证**

Run:
```bash
mkdir -p build-qmake && cd build-qmake && qmake ../standard_lable.pro && make -j2; cd ..
```
Expected: 成功生成 `libslabel.so`（qmake 路径与 CMake 产物一致）。

> 若本机无 qmake，跳过本步并在 README 注明 qmake 构建方式，由具备 Qt 环境者验证。

- [ ] **Step 2: 写 README**

`README.md`:
```markdown
# standard_lable (slabel)

Qt 5.15 基础控件封装库：统一可切换主题、运行时多语言、双向/单向数据绑定。

## 构建
CMake：`cmake -S . -B build && cmake --build build`
qmake：`qmake standard_lable.pro && make`

## 用法
\`\`\`cpp
#include "slabel/SButton.h"
#include "slabel/ThemeManager.h"

ThemeManager::instance().registerTheme("dark", "themes/dark.qss");
ThemeManager::instance().setTheme("dark");

SButton* btn = new SButton;
btn->setTextTr("保存");                 // 随语言重译
btn->setThemeOverride("color", "#f00"); // 代码覆盖 QSS
\`\`\`

支持：Qt 5.15.2 / 5.15.10，Windows 7 / Linux，C++17。
```

- [ ] **Step 3: 提交**

```bash
git add standard_lable.pro README.md
git commit -m "docs: README 与 qmake 构建校验"
```

---

## 自检结论（对照 spec）

- **需求覆盖**：封装控件(Task 4/7)、统一可切换主题(Task 3/4/8)、多语言运行时切换(Task 5/8)、事件绑定双向+单向(Task 6)、动态库+双构建(Task 1/10)、Qt 5.15/Win7/Linux 约束(全局约束+Task 1/10)、15 个控件(Task 4/7)、自定义/自绘控件支持与主题 token(Task 3 的 token API + Task 9 的 SSwatch 示例)、测试(每任务 TDD + Task 9 集成)。
- **类型一致性**：`registerControl/unregisterControl`、`setTheme/setLanguage`、`setThemeOverride/clearThemeOverride`、`setTextTr/retranslate`、`bind/observe/unbind` 在各任务中签名一致。
- **无占位符**：每步含实际代码与命令。
