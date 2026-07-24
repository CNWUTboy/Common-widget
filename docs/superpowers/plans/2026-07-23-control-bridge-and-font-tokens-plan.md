# SControlBridge 桥接机制与字号角色化 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让业务层已写好的自定义控件类（无法改成继承 `SControl<Base>`）也能获得主题/语言/操作状态能力，同时把主题 QSS 的字号控制从单一 `@fontSizeBase` 拆成可独立调整的按角色 token。

**Architecture:** 把 `SControl<Base>` 里与具体控件类型无关的部分（主题覆盖表 + 操作状态机）抽成独立的 `SControlEngine`（非 `QObject`，持有一个不拥有所有权的 `QWidget*` 目标）。`SControl<Base>` 改为持有 `SControlEngine` 成员并转发；新增的 `SControlBridge`（`QObject` + `ISControl`）同样持有一个 `SControlEngine` 成员，通过组合（`SControlBridge m_bridge{this};` 成员）或 `slabelAttach(widget)` 自由函数两种方式挂到已有控件上。`SControlEngine` 不实现 `ISControl`、不自行向 `ThemeManager`/`LanguageManager` 注册——注册和 `asWidget()`/`retranslate()` 契约留给持有者。字号改动只是给每条 `[slabelRole=...]` QSS 规则补一个独立的 `font-size:@fontSizeXxx` token，初始值等于原 `@fontSizeBase`，零视觉变化。

**Tech Stack:** Qt 5.15 Widgets, C++17（CRTP + SFINAE + `if constexpr`），CMake + qmake 双构建系统，QtTest。

## Global Constraints

- 支持 Qt 5.15.2 / 5.15.10，Windows 7 / Linux，C++17（沿用 README 已声明的要求）。
- `SControl<Base>` 现有的 `retranslate`/`setTextTr`/`changeEvent`/`HasSetText` 编译期分支保持原样，不下沉进 `SControlEngine`——零性能影响，15 个内置控件的现有路径不变。
- 不发明任何新的角色字号像素值：所有新增 `@fontSizeXxx` token 初始值必须等于该主题原有的 `@fontSizeBase`（`default.qss`/`light.qss` = `13px`，`dark.qss` = `11px`）。
- 不新增从 `QWidget*` 反查 `SControlBridge*` 的 API（YAGNI）。
- 不改动 `ThemeManager`/`LanguageManager` 的注册结构或公开 API。
- 不为重复 `slabelAttach()` 同一个 widget 加去重/守卫逻辑（调用方自己负责，和 `SControl<Base>` 单次构造的语义一致）。
- 每个任务完成后，除新增测试外，`tests/test_language.cpp`、`tests/test_scontrol_theme.cpp`、`tests/test_controls.cpp`、`tests/test_opstate.cpp`、`tests/test_controlcore.cpp`、`tests/test_thememanager.cpp`、`tests/test_binding.cpp`、`tests/test_theme_files.cpp` 必须原样通过（不修改这些文件）。

---

### Task 1: 抽取 SControlEngine，改造 SControl\<Base\> 为转发

**Files:**
- Create: `include/slabel/SControlEngine.h`
- Create: `src/SControlEngine.cpp`
- Modify: `include/slabel/SControl.h`（整体重写，176 行 → 更短的转发版本）
- Modify: `CMakeLists.txt:16-35`（`SLABEL_SOURCES`/`SLABEL_HEADERS` 追加两个新文件）

**Interfaces:**
- Consumes: `SControlCore`（`include/slabel/SControlCore.h`，已存在：`notifyOperationStateChanged()`、`operationStateChanged` 信号）、`OperationState`（`include/slabel/OperationState.h`，已存在枚举）。
- Produces（后续任务都依赖这些确切签名）：
  - `class SControlEngine`：`explicit SControlEngine(QWidget* widget)`；`void setThemeOverride(const QString&, const QString&)`；`void clearThemeOverride()`；`void setFontSizePx(int px)`；`SControlCore& core()`；`void setOperationHandler(std::function<void()>)`；`void reportOperationResult(bool)`；`OperationState operationState() const`；`void resetOperationState()`；`void setOperationTimeoutMs(int)`；`void setOperationResetDelayMs(int)`；`void triggerOperation()`。
  - `SControl<Base>` 保留原有全部公开方法签名不变，仅内部实现变为转发到 `m_engine`。

- [ ] **Step 1: 跑一次现有测试套件，确认改造前的基线是绿的**

Run:
```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```
Expected: 全部现有测试（`test_controlcore`、`test_thememanager`、`test_scontrol_theme`、`test_language`、`test_binding`、`test_controls`、`test_theme_files`、`test_opstate`）PASS。这是本次重构"零行为变化"的基线，后面每步都要保持全绿。

- [ ] **Step 2: 新建 `include/slabel/SControlEngine.h`**

```cpp
#pragma once

#include <QWidget>
#include <QHash>
#include <QString>
#include <QTimer>
#include <functional>
#include "slabel/SGlobal.h"
#include "slabel/SControlCore.h"
#include "slabel/OperationState.h"

// 从 SControl<Base> 中抽取的、与具体控件类型无关的机制：主题覆盖表 + 操作
// 状态机。不含文本重译（HasSetText 编译期分支留在 SControl<Base> 内，
// SControlBridge 走运行时属性查找的独立实现），也不实现 ISControl、不向
// ThemeManager/LanguageManager 注册——这些契约由持有者（SControl<Base> 或
// SControlBridge）负责，避免重复 retranslate() 调用、破坏 asWidget() 语义。
class SLABEL_EXPORT SControlEngine {
public:
    explicit SControlEngine(QWidget* widget);
    ~SControlEngine();

    // 主题：代码覆盖（widget 级 QSS，优先级高于应用级）
    void setThemeOverride(const QString& key, const QString& value);
    void clearThemeOverride();
    // 复用覆盖表实现的单独字号入口：等价于 setThemeOverride("font-size", "20px")
    void setFontSizePx(int px);

    SControlCore& core() { return m_core; }  // 暴露 operationStateChanged 等信号

    void setOperationHandler(std::function<void()> handler);
    // 结果回传：仅在"执行中"时生效，迟到/多余的回报会被忽略
    void reportOperationResult(bool success);
    OperationState operationState() const { return m_opState; }
    // 复位：仅在"成功"/"失败"时生效，提前恢复为"待命"
    void resetOperationState();
    void setOperationTimeoutMs(int ms) { m_opTimeoutMs = ms; }
    void setOperationResetDelayMs(int ms) { m_opResetDelayMs = ms; }
    void triggerOperation();

private:
    void applyOverrides();
    void setOpState(OperationState s);
    void applyOperationVisual();
    void onOperationTimeout();
    void onOperationResetTimeout();

    QWidget* m_widget;  // 非拥有：目标控件的生命周期由持有者管理
    SControlCore m_core;
    QHash<QString, QString> m_overrides;
    OperationState m_opState = OperationState::Idle;
    std::function<void()> m_opHandler;
    // SControlEngine 本身不是 QObject，定时器改挂到 m_core（先于它们构造，
    // 且生命周期与引擎一致）而不是 m_widget 上。
    QTimer m_opBusyTimer{&m_core};
    QTimer m_opResetTimer{&m_core};
    int m_opTimeoutMs = 10000;
    int m_opResetDelayMs = 2000;
};
```

- [ ] **Step 3: 新建 `src/SControlEngine.cpp`**

```cpp
#include "slabel/SControlEngine.h"
#include <QStyle>
#include <QVariant>

SControlEngine::SControlEngine(QWidget* widget) : m_widget(widget) {
    m_opBusyTimer.setSingleShot(true);
    m_opResetTimer.setSingleShot(true);
    QObject::connect(&m_opBusyTimer, &QTimer::timeout, &m_core, [this]{ onOperationTimeout(); });
    QObject::connect(&m_opResetTimer, &QTimer::timeout, &m_core, [this]{ onOperationResetTimeout(); });
}

SControlEngine::~SControlEngine() = default;

void SControlEngine::setThemeOverride(const QString& key, const QString& value) {
    m_overrides.insert(key, value);
    applyOverrides();
}

void SControlEngine::clearThemeOverride() {
    m_overrides.clear();
    applyOverrides();
}

void SControlEngine::setFontSizePx(int px) {
    setThemeOverride(QStringLiteral("font-size"), QStringLiteral("%1px").arg(px));
}

void SControlEngine::applyOverrides() {
    if (m_overrides.isEmpty()) {
        m_widget->setStyleSheet(QString());
        return;
    }
    QString body;
    for (auto it = m_overrides.constBegin(); it != m_overrides.constEnd(); ++it)
        body += it.key() + QStringLiteral(":") + it.value() + QStringLiteral(";");
    // widget 级样式表以自身为选择器，天然覆盖应用级 QSS
    m_widget->setStyleSheet(QStringLiteral("* {") + body + QStringLiteral("}"));
}

void SControlEngine::setOperationHandler(std::function<void()> handler) {
    m_opHandler = std::move(handler);
}

void SControlEngine::reportOperationResult(bool success) {
    if (m_opState != OperationState::Busy) return;
    m_opBusyTimer.stop();
    setOpState(success ? OperationState::Success : OperationState::Failure);
    if (m_opResetDelayMs > 0) m_opResetTimer.start(m_opResetDelayMs);
}

void SControlEngine::resetOperationState() {
    if (m_opState != OperationState::Success && m_opState != OperationState::Failure) return;
    m_opResetTimer.stop();
    setOpState(OperationState::Idle);
}

void SControlEngine::triggerOperation() {
    if (m_opState == OperationState::Busy) return; // 重复触发不产生新操作
    // 未登记处理方式时，视为该控件未接入操作能力，点击保持原生行为
    if (!m_opHandler) return;
    setOpState(OperationState::Busy);
    if (m_opTimeoutMs > 0) m_opBusyTimer.start(m_opTimeoutMs);
    m_opHandler();
}

void SControlEngine::setOpState(OperationState s) {
    m_opState = s;
    applyOperationVisual();
    m_core.notifyOperationStateChanged();
}

// 默认视觉反馈：写入 Qt 动态属性，交由主题 QSS 的属性选择器呈现
void SControlEngine::applyOperationVisual() {
    switch (m_opState) {
        case OperationState::Busy:    m_widget->setProperty("slabelOperationState", QStringLiteral("busy")); break;
        case OperationState::Success: m_widget->setProperty("slabelOperationState", QStringLiteral("success")); break;
        case OperationState::Failure: m_widget->setProperty("slabelOperationState", QStringLiteral("failure")); break;
        default:                      m_widget->setProperty("slabelOperationState", QVariant()); break;
    }
    m_widget->style()->unpolish(m_widget);
    m_widget->style()->polish(m_widget);
    m_widget->update();
}

void SControlEngine::onOperationTimeout() {
    if (m_opState == OperationState::Busy) {
        setOpState(OperationState::Failure);
        if (m_opResetDelayMs > 0) m_opResetTimer.start(m_opResetDelayMs);
    }
}

void SControlEngine::onOperationResetTimeout() {
    if (m_opState == OperationState::Success || m_opState == OperationState::Failure)
        setOpState(OperationState::Idle);
}
```

- [ ] **Step 4: 把新文件加进 `CMakeLists.txt`**

在 `CMakeLists.txt:16-23`（`SLABEL_SOURCES`）里，`src/SControlCore.cpp` 后面插入 `src/SControlEngine.cpp`：

```cmake
set(SLABEL_SOURCES
    src/BindingEngine.cpp
    src/LanguageManager.cpp
    src/SBindable.cpp
    src/SControlCore.cpp
    src/SControlEngine.cpp
    src/slabel_version.cpp
    src/ThemeManager.cpp
)
```

在 `CMakeLists.txt:25-35`（`SLABEL_HEADERS`）里，`include/slabel/SControlCore.h` 后面插入 `include/slabel/SControlEngine.h`：

```cmake
set(SLABEL_HEADERS
    include/slabel/BindingEngine.h
    include/slabel/ISControl.h
    include/slabel/LanguageManager.h
    include/slabel/OperationState.h
    include/slabel/SBindable.h
    include/slabel/SControl.h
    include/slabel/SControlCore.h
    include/slabel/SControlEngine.h
    include/slabel/SGlobal.h
    include/slabel/ThemeManager.h
)
```

qmake 端（`standard_lable.pro`）用 `$$files(include/slabel/*.h)` / `$$files(src/*.cpp)` 通配收集源文件，新文件会自动被拾取，不需要改动。

- [ ] **Step 5: 构建，确认新引擎能独立编译进库（此时还没被使用）**

Run:
```bash
cmake --build build -j"$(nproc)"
```
Expected: 编译成功，无报错（`SControlEngine` 此刻是死代码，但应能正常编译进 `libslabel`）。

- [ ] **Step 6: 重写 `include/slabel/SControl.h` 为转发版本**

完整替换文件内容为：

```cpp
#pragma once
#include <QWidget>
#include <QByteArray>
#include <QCoreApplication>
#include <QEvent>
#include <functional>
#include <type_traits>
#include <utility>
#include "slabel/ISControl.h"
#include "slabel/SControlCore.h"
#include "slabel/SControlEngine.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"
#include "slabel/OperationState.h"

namespace slabel_detail {
// 探测 Base 是否具备 setText(QString) —— 无文本控件（如 QComboBox、
// QSpinBox、QTableView 等）没有此接口，retranslate() 需据此在编译期分支，
// 否则虚函数隐式实例化时会因找不到 setText 而编译失败。
template<class T, class = void>
struct HasSetText : std::false_type {};
template<class T>
struct HasSetText<T, std::void_t<decltype(std::declval<T&>().setText(QString()))>>
    : std::true_type {};
}

// CRTP 能力模板：把主题/语言/绑定/操作状态挂钩写一次，套到任意 Qt 基类上。
// 注意：模板类不含 Q_OBJECT；signal 由 m_engine.core() 提供。
template<class Base>
class SControl : public Base, public ISControl {
public:
    // 转发引用构造：为避免以非 const 左值调用时把拷贝/移动构造"劫持"掉
    // （从而产生费解的深层模板报错），对单参数且该参数本身是 SControl
    // 派生/同类的情况做 SFINAE 排除，让编译器改选（被删除的）拷贝构造，
    // 报错更清晰。
    template<typename... Args,
             typename = std::enable_if_t<
                 !(sizeof...(Args) == 1 &&
                   (std::is_base_of_v<SControl, std::decay_t<Args>> || ...))>>
    explicit SControl(Args&&... args) : Base(std::forward<Args>(args)...), m_engine(this) {
        ThemeManager::instance().registerControl(this);
        LanguageManager::instance().registerControl(this);
    }
    ~SControl() override {
        ThemeManager::instance().unregisterControl(this);
        LanguageManager::instance().unregisterControl(this);
    }

    QWidget* asWidget() override { return this; }

    // 语言：记住源串，切换时自动重译
    void setTextTr(const char* sourceText) {
        m_sourceText = QByteArray(sourceText);
        retranslate();
    }
    void retranslate() override {
        if constexpr (slabel_detail::HasSetText<Base>::value) {
            if (!m_sourceText.isEmpty())
                this->setText(QCoreApplication::translate("slabel", m_sourceText.constData()));
        }
    }

    // 主题：代码覆盖（widget 级 QSS，优先级高于应用级）——转发到引擎
    void setThemeOverride(const QString& key, const QString& value) { m_engine.setThemeOverride(key, value); }
    void clearThemeOverride() { m_engine.clearThemeOverride(); }
    void setFontSizePx(int px) { m_engine.setFontSizePx(px); }

    // 操作状态反馈能力（RX_OP_STATE）——转发到引擎
    SControlCore& core() { return m_engine.core(); }

    void setOperationHandler(std::function<void()> handler) { m_engine.setOperationHandler(std::move(handler)); }
    void reportOperationResult(bool success) { m_engine.reportOperationResult(success); }
    OperationState operationState() const { return m_engine.operationState(); }
    void resetOperationState() { m_engine.resetOperationState(); }
    void setOperationTimeoutMs(int ms) { m_engine.setOperationTimeoutMs(ms); }
    void setOperationResetDelayMs(int ms) { m_engine.setOperationResetDelayMs(ms); }

    // 具体控件（或自定义控件，含业务层直接实例化的 SControl<T>）把自己的
    // "触发"信号接到这里即可接入操作状态反馈能力，无需继承。
    void triggerOperation() { m_engine.triggerOperation(); }

protected:
    // 语言切换自动重译：任意 Base 通用，业务层直接用 SControl<T> 无需子类
    // 重写 changeEvent 即可获得该能力。
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        Base::changeEvent(e);
    }

    SControlEngine m_engine;
    QByteArray m_sourceText;
};
```

- [ ] **Step 7: 重新构建并跑全部测试，确认零行为变化**

Run:
```bash
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```
Expected: Step 1 里跑过的全部测试仍然 PASS，一个都不能少、不能改。如果 `test_opstate.cpp` 的 `destroyingBusyControlDoesNotCrash` 或任何用例失败，说明引擎抽取破坏了原有的定时器/析构顺序假设，需要回到 Step 6 检查 `SControlEngine` 成员声明顺序（必须是 `m_widget` → `m_core` → `m_overrides` → `m_opState` → `m_opHandler` → `m_opBusyTimer` → `m_opResetTimer` → `m_opTimeoutMs` → `m_opResetDelayMs`，定时器的默认成员初始化 `{&m_core}` 依赖 `m_core` 先于它们构造）。

- [ ] **Step 8: Commit**

```bash
git add include/slabel/SControlEngine.h src/SControlEngine.cpp include/slabel/SControl.h CMakeLists.txt
git commit -m "refactor: 抽取 SControlEngine，SControl<Base> 改为转发（零行为变化）"
```

---

### Task 2: SControlBridge 组合式桥接 + slabelAttach

**Files:**
- Create: `include/slabel/SControlBridge.h`
- Create: `src/SControlBridge.cpp`
- Create: `tests/test_control_bridge.cpp`
- Create: `tests/test_control_bridge.pro`
- Modify: `CMakeLists.txt`（`SLABEL_SOURCES`/`SLABEL_HEADERS` 追加两个新文件）
- Modify: `tests/CMakeLists.txt`（追加 `slabel_test(test_control_bridge)`）
- Modify: `tests/tests.pro`（`SUBDIRS` 追加 `test_control_bridge`）

**Interfaces:**
- Consumes: `SControlEngine`（Task 1 产出，构造签名 `SControlEngine(QWidget*)`，方法集见 Task 1 的 Produces）、`ISControl`（`include/slabel/ISControl.h`：`virtual QWidget* asWidget() = 0; virtual void retranslate() = 0;`）。
- Produces：
  - `class SControlBridge : public QObject, public ISControl`：`explicit SControlBridge(QWidget* target, QObject* parent = nullptr)`；其余方法签名与 `SControl<Base>` 的对应方法一致（同名同参）。
  - `SControlBridge* slabelAttach(QWidget* widget)` 自由函数。

- [ ] **Step 1: 写失败测试 `tests/test_control_bridge.cpp`**

```cpp
#include <QtTest>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QCoreApplication>
#include "slabel/SControlBridge.h"

// 组合式用法的参考实现：业务层自定义控件类，把 SControlBridge 当成员用。
class BridgedLabel : public QWidget {
public:
    explicit BridgedLabel(QWidget* parent = nullptr)
        : QWidget(parent), label(this), bridge(&label) {}
    QLabel label;
    SControlBridge bridge;
};

class TestControlBridge : public QObject {
    Q_OBJECT
private slots:
    void composedMemberSetTextTrAppliesToTarget() {
        BridgedLabel w;
        w.bridge.setTextTr("Save");
        QCOMPARE(w.label.text(), QString("Save"));
        w.bridge.retranslate();
        QCOMPARE(w.label.text(), QString("Save"));
    }
    void changeEventOnTargetTriggersRetranslateViaEventFilter() {
        // 验证 SControlBridge 用 installEventFilter 替代 changeEvent 重写
        // （因为它不是目标 QWidget 本身）依然能捕获语言切换事件。
        QLabel label;
        SControlBridge bridge(&label);
        bridge.setTextTr("Save");
        QCOMPARE(label.text(), QString("Save"));
        label.setText(QStringLiteral("mutated"));
        QEvent ev(QEvent::LanguageChange);
        QCoreApplication::sendEvent(&label, &ev);
        QCOMPARE(label.text(), QString("Save"));
    }
    void themeOverrideAffectsTargetStyleSheet() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        bridge.setThemeOverride("color", "#ff0000");
        QVERIFY(btn.styleSheet().contains("color:#ff0000"));
        bridge.clearThemeOverride();
        QVERIFY(btn.styleSheet().isEmpty());
    }
    void operationStateMachineSmokeTestViaBridge() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        int calls = 0;
        bridge.setOperationHandler([&]{ ++calls; });
        bridge.triggerOperation();
        QCOMPARE(calls, 1);
        QCOMPARE(int(bridge.operationState()), int(OperationState::Busy));
    }
    void attachReturnsNonNullAndAutoDestroysWithWidget() {
        auto* widget = new QPushButton;
        SControlBridge* bridge = slabelAttach(widget);
        QVERIFY(bridge != nullptr);
        QSignalSpy spy(bridge, &QObject::destroyed);
        delete widget;
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestControlBridge)
#include "test_control_bridge.moc"
```

- [ ] **Step 2: 新建 `tests/test_control_bridge.pro`**

```
QT += testlib widgets
TEMPLATE = app
CONFIG += c++17 testcase console
TARGET = test_control_bridge

INCLUDEPATH += ../include
LIBS += -L../bin -lslabel

SOURCES = test_control_bridge.cpp
```

- [ ] **Step 3: 把新测试注册进两套构建系统**

在 `tests/CMakeLists.txt` 末尾（`slabel_test(test_opstate)` 后面）追加：

```cmake
slabel_test(test_control_bridge)
```

在 `tests/tests.pro` 的 `SUBDIRS` 列表末尾（`test_opstate` 后面）追加：

```
TEMPLATE = subdirs

SUBDIRS = \
    test_controlcore \
    test_thememanager \
    test_scontrol_theme \
    test_language \
    test_binding \
    test_controls \
    test_theme_files \
    test_opstate \
    test_control_bridge
```

- [ ] **Step 4: 构建，确认新测试因为 `SControlBridge.h` 不存在而编译失败**

Run:
```bash
cmake -S . -B build
cmake --build build --target test_control_bridge
```
Expected: FAIL，报错找不到 `slabel/SControlBridge.h`（`fatal error: slabel/SControlBridge.h: No such file or directory`）。

- [ ] **Step 5: 新建 `include/slabel/SControlBridge.h`**

```cpp
#pragma once
#include <QObject>
#include <QWidget>
#include <QString>
#include <QByteArray>
#include <QEvent>
#include <functional>
#include "slabel/SGlobal.h"
#include "slabel/ISControl.h"
#include "slabel/SControlCore.h"
#include "slabel/SControlEngine.h"
#include "slabel/OperationState.h"

// 面向"已经写好、不方便改成继承 SControl<Base>"的自定义控件类的运行时桥接：
// 组合一个 SControlBridge 成员（构造时传入目标 QWidget*）即可获得主题/语言/
// 操作状态能力，或用下面的 slabelAttach() 自由函数零侵入地挂到任意已有控件上。
class SLABEL_EXPORT SControlBridge : public QObject, public ISControl {
    Q_OBJECT
public:
    explicit SControlBridge(QWidget* target, QObject* parent = nullptr);
    ~SControlBridge() override;

    QWidget* asWidget() override { return m_target; }

    // 语言：与 SControl<Base> 语义一致，但走运行时属性查找（不依赖编译期
    // HasSetText），因为目标类型在这里是不透明的 QWidget*。
    void setTextTr(const char* sourceText);
    void retranslate() override;

    void setThemeOverride(const QString& key, const QString& value) { m_engine.setThemeOverride(key, value); }
    void clearThemeOverride() { m_engine.clearThemeOverride(); }
    void setFontSizePx(int px) { m_engine.setFontSizePx(px); }

    SControlCore& core() { return m_engine.core(); }

    void setOperationHandler(std::function<void()> handler) { m_engine.setOperationHandler(std::move(handler)); }
    void reportOperationResult(bool success) { m_engine.reportOperationResult(success); }
    OperationState operationState() const { return m_engine.operationState(); }
    void resetOperationState() { m_engine.resetOperationState(); }
    void setOperationTimeoutMs(int ms) { m_engine.setOperationTimeoutMs(ms); }
    void setOperationResetDelayMs(int ms) { m_engine.setOperationResetDelayMs(ms); }
    void triggerOperation() { m_engine.triggerOperation(); }

protected:
    // SControlBridge 不是目标 widget 本身，不能重写 changeEvent；改用事件
    // 过滤器观察目标的 QEvent::LanguageChange。
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    QWidget* m_target;  // 非拥有
    SControlEngine m_engine;
    QByteArray m_sourceText;
};

// 零侵入挂接：堆分配，parent = widget，随 widget 的 Qt 父子链自动析构。
SLABEL_EXPORT SControlBridge* slabelAttach(QWidget* widget);
```

- [ ] **Step 6: 新建 `src/SControlBridge.cpp`**

```cpp
#include "slabel/SControlBridge.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"
#include <QMetaObject>
#include <QCoreApplication>

SControlBridge::SControlBridge(QWidget* target, QObject* parent)
    : QObject(parent), m_target(target), m_engine(target) {
    ThemeManager::instance().registerControl(this);
    LanguageManager::instance().registerControl(this);
    m_target->installEventFilter(this);
}

SControlBridge::~SControlBridge() {
    ThemeManager::instance().unregisterControl(this);
    LanguageManager::instance().unregisterControl(this);
}

void SControlBridge::setTextTr(const char* sourceText) {
    m_sourceText = QByteArray(sourceText);
    retranslate();
}

void SControlBridge::retranslate() {
    if (m_sourceText.isEmpty()) return;
    // 运行时属性查找：目标类型不透明，靠 QMetaObject 判断是否有 "text" 属性
    // （QLabel/QPushButton/QLineEdit 等有文本控件都有），没有则安全跳过。
    if (m_target->metaObject()->indexOfProperty("text") < 0) return;
    m_target->setProperty("text", QCoreApplication::translate("slabel", m_sourceText.constData()));
}

bool SControlBridge::eventFilter(QObject* obj, QEvent* e) {
    if (obj == m_target && e->type() == QEvent::LanguageChange) retranslate();
    return QObject::eventFilter(obj, e);
}

SControlBridge* slabelAttach(QWidget* widget) {
    return new SControlBridge(widget, widget);
}
```

- [ ] **Step 7: 把新文件加进 `CMakeLists.txt`**

在 `CMakeLists.txt` 的 `SLABEL_SOURCES` 里，`src/SBindable.cpp` 后面插入 `src/SControlBridge.cpp`（字母序 Bridge < Core < Engine）：

```cmake
set(SLABEL_SOURCES
    src/BindingEngine.cpp
    src/LanguageManager.cpp
    src/SBindable.cpp
    src/SControlBridge.cpp
    src/SControlCore.cpp
    src/SControlEngine.cpp
    src/slabel_version.cpp
    src/ThemeManager.cpp
)
```

在 `SLABEL_HEADERS` 里，`include/slabel/SControl.h` 后面插入 `include/slabel/SControlBridge.h`：

```cmake
set(SLABEL_HEADERS
    include/slabel/BindingEngine.h
    include/slabel/ISControl.h
    include/slabel/LanguageManager.h
    include/slabel/OperationState.h
    include/slabel/SBindable.h
    include/slabel/SControl.h
    include/slabel/SControlBridge.h
    include/slabel/SControlCore.h
    include/slabel/SControlEngine.h
    include/slabel/SGlobal.h
    include/slabel/ThemeManager.h
)
```

- [ ] **Step 8: 构建并运行新测试，确认 PASS**

Run:
```bash
cmake --build build -j"$(nproc)"
ctest --test-dir build -R test_control_bridge --output-on-failure
```
Expected: `test_control_bridge` 的 5 个用例全部 PASS。

- [ ] **Step 9: 跑全量回归测试**

Run:
```bash
ctest --test-dir build --output-on-failure
```
Expected: 全部测试（含 Task 1 基线里的所有旧测试）PASS。

- [ ] **Step 10: Commit**

```bash
git add include/slabel/SControlBridge.h src/SControlBridge.cpp tests/test_control_bridge.cpp tests/test_control_bridge.pro CMakeLists.txt tests/CMakeLists.txt tests/tests.pro
git commit -m "feat: 新增 SControlBridge 运行时桥接 + slabelAttach，供已有控件类零改造接入 slabel 能力"
```

---

### Task 3: 主题 QSS 补充角色字号 token + setFontSizePx 回归测试

**Files:**
- Modify: `themes/default.qss`
- Modify: `themes/dark.qss`
- Modify: `themes/light.qss`
- Create: `tests/test_font_tokens.cpp`
- Create: `tests/test_font_tokens.pro`
- Modify: `tests/CMakeLists.txt`（追加 `slabel_test(test_font_tokens)` + `THEME_DIR` 编译定义）
- Modify: `tests/tests.pro`（`SUBDIRS` 追加 `test_font_tokens`）

**Interfaces:**
- Consumes: `SControl<QPushButton>::setFontSizePx(int)`（Task 1 产出，转发到 `SControlEngine::setFontSizePx`）、`ThemeManager`（已存在，不改动）。
- Produces: 无后续任务依赖此任务的产出（终节点任务）。

- [ ] **Step 1: 写失败测试 `tests/test_font_tokens.cpp`**

```cpp
#include <QtTest>
#include <QFile>
#include <QPushButton>
#include "slabel/SControl.h"

class TestFontTokens : public QObject {
    Q_OBJECT
private slots:
    void primaryButtonRuleUsesDistinctFontSizeToken() {
        QFile f(QStringLiteral(THEME_DIR "/default.qss"));
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString text = QString::fromUtf8(f.readAll());
        const int idx = text.indexOf(QStringLiteral("*[slabelRole=\"primaryButton\"]"));
        QVERIFY(idx >= 0);
        const int end = text.indexOf(QChar('}'), idx);
        QVERIFY(end > idx);
        const QString rule = text.mid(idx, end - idx);
        // 角色规则必须有自己独立的字号 token（不是复用 @fontSizeBase）
        QVERIFY(rule.contains(QStringLiteral("font-size:@fontSizeButton")));
    }
    void setFontSizePxAppliesInlineOverride() {
        SControl<QPushButton> btn;
        btn.setFontSizePx(20);
        QVERIFY(btn.styleSheet().contains(QStringLiteral("font-size:20px")));
    }
};

QTEST_MAIN(TestFontTokens)
#include "test_font_tokens.moc"
```

- [ ] **Step 2: 新建 `tests/test_font_tokens.pro`**

```
QT += testlib widgets
TEMPLATE = app
CONFIG += c++17 testcase console
TARGET = test_font_tokens

INCLUDEPATH += ../include
LIBS += -L../bin -lslabel

SOURCES = test_font_tokens.cpp

# 告诉测试代码主题文件所在目录（与 CMake 的 THEME_DIR 等效）
DEFINES += THEME_DIR=\\\"$$shell_path($${_PRO_FILE_PWD_}/../build/themes)\\\"
```

- [ ] **Step 3: 把新测试注册进两套构建系统**

在 `tests/CMakeLists.txt` 末尾（`slabel_test(test_control_bridge)` 后面）追加：

```cmake
slabel_test(test_font_tokens)
target_compile_definitions(test_font_tokens PRIVATE THEME_DIR="${CMAKE_BINARY_DIR}/themes")
```

在 `tests/tests.pro` 的 `SUBDIRS` 列表末尾（`test_control_bridge` 后面）追加：

```
TEMPLATE = subdirs

SUBDIRS = \
    test_controlcore \
    test_thememanager \
    test_scontrol_theme \
    test_language \
    test_binding \
    test_controls \
    test_theme_files \
    test_opstate \
    test_control_bridge \
    test_font_tokens
```

- [ ] **Step 4: 构建并运行，确认 `primaryButtonRuleUsesDistinctFontSizeToken` 失败（QSS 还没改）**

Run:
```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
ctest --test-dir build -R test_font_tokens --output-on-failure
```
Expected: `setFontSizePxAppliesInlineOverride` PASS（`setFontSizePx` 在 Task 1 已实现），`primaryButtonRuleUsesDistinctFontSizeToken` FAIL（`QVERIFY(rule.contains(...))` 断言失败，因为 `default.qss` 里 `primaryButton` 规则还没有 `font-size:@fontSizeButton`）。

- [ ] **Step 5: 修改 `themes/default.qss`**

完整替换文件内容为：

```
/* @primary:#0d6efd @bg:#ffffff @text:#212529 @border:#ced4da
   @fontFamily:"Segoe UI" @fontSizeBase:13px
   @fontSizeButton:13px @fontSizeInput:13px @fontSizeCombo:13px
   @fontSizeCheckBox:13px @fontSizeRadioButton:13px @fontSizeGroupBox:13px @fontSizeProgressBar:13px
   @iconSave:":/icons/light/save.svg" */

* { font-family:@fontFamily; font-size:@fontSizeBase; }

*[slabelRole="primaryButton"] { background:@primary; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; font-size:@fontSizeButton; qproperty-icon: url(@iconSave); }
*[slabelRole="textInput"]     { background:@bg; color:@text; border:1px solid @border; border-radius:4px; padding:4px; font-size:@fontSizeInput; }
*[slabelRole="comboBox"]      { background:@bg; color:@text; border:1px solid @border; border-radius:4px; font-size:@fontSizeCombo; }
*[slabelRole="checkBox"]      { color:@text; font-size:@fontSizeCheckBox; }
*[slabelRole="radioButton"]   { color:@text; font-size:@fontSizeRadioButton; }
*[slabelRole="groupBox"]      { color:@text; border:1px solid @border; border-radius:4px; margin-top:6px; font-size:@fontSizeGroupBox; }
*[slabelRole="progressBar"]   { border:1px solid @border; border-radius:4px; text-align:center; font-size:@fontSizeProgressBar; }
*[slabelRole="progressBar"]::chunk { background:@primary; }

*[slabelOperationState="busy"]    { background:#6c757d; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; }
*[slabelOperationState="success"] { background:#28a745; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; }
*[slabelOperationState="failure"] { background:#dc3545; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; }
```

- [ ] **Step 6: 修改 `themes/dark.qss`**

完整替换文件内容为：

```
/* @primary:#3d0000 @bg:#101010 @text:#ffffff @border:#4d4d4d
   @fontFamily:"Microsoft YaHei" @fontSizeBase:11px
   @fontSizeButton:11px @fontSizeInput:11px @fontSizeCombo:11px
   @fontSizeCheckBox:11px @fontSizeRadioButton:11px @fontSizeGroupBox:11px @fontSizeProgressBar:11px
   @iconSave:":/icons/dark/save.svg" */

* { font-family:@fontFamily; font-size:@fontSizeBase; }

*[slabelRole="primaryButton"] { background:@primary; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; font-size:@fontSizeButton; qproperty-icon: url(@iconSave); }
*[slabelRole="textInput"]     { background:@bg; color:@text; border:1px solid @border; border-radius:4px; padding:4px; font-size:@fontSizeInput; }
*[slabelRole="comboBox"]      { background:@bg; color:@text; border:1px solid @border; border-radius:4px; font-size:@fontSizeCombo; }
*[slabelRole="checkBox"]      { background:@bg; color:@text; font-size:@fontSizeCheckBox; }
*[slabelRole="radioButton"]   { background:@bg; color:@text; font-size:@fontSizeRadioButton; }
*[slabelRole="groupBox"]      { background:@bg;  color:@text; border:1px solid @border; border-radius:4px; margin-top:6px; font-size:@fontSizeGroupBox; }
*[slabelRole="progressBar"]   { color:@text;  border:1px solid @border; border-radius:4px; text-align:center; font-size:@fontSizeProgressBar; }
*[slabelRole="progressBar"]::chunk { background:@primary; }

*[slabelOperationState="busy"]    { background:#5a5f63; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
*[slabelOperationState="success"] { background:#2fa84f; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
*[slabelOperationState="failure"] { background:#c94f4f; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
```

- [ ] **Step 7: 修改 `themes/light.qss`**

完整替换文件内容为：

```
/* @primary:#00ffff @bg:#00cccc @text:#000000 @border:#4d4d4d
   @fontFamily:"Microsoft YaHei" @fontSizeBase:13px
   @fontSizeButton:13px @fontSizeInput:13px @fontSizeCombo:13px
   @fontSizeCheckBox:13px @fontSizeRadioButton:13px @fontSizeGroupBox:13px @fontSizeProgressBar:13px
   @iconSave:":/icons/dark/save.svg" */

* { font-family:@fontFamily; font-size:@fontSizeBase; }

*[slabelRole="primaryButton"] { background:@primary; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; font-size:@fontSizeButton; qproperty-icon: url(@iconSave); }
*[slabelRole="textInput"]     { background:@bg; color:@text; border:1px solid @border; border-radius:4px; padding:4px; font-size:@fontSizeInput; }
*[slabelRole="comboBox"]      { background:@bg; color:@text; border:1px solid @border; border-radius:4px; font-size:@fontSizeCombo; }
*[slabelRole="checkBox"]      { background:@bg; color:@text; font-size:@fontSizeCheckBox; }
*[slabelRole="radioButton"]   { background:@bg; color:@text; font-size:@fontSizeRadioButton; }
*[slabelRole="groupBox"]      { background:@bg; color:@text; border:1px solid @border; border-radius:4px; margin-top:6px; font-size:@fontSizeGroupBox; }
*[slabelRole="progressBar"]   { color:@text;  border:1px solid @border; border-radius:4px; text-align:center; font-size:@fontSizeProgressBar; }
*[slabelRole="progressBar"]::chunk { background:@primary; }

*[slabelOperationState="busy"]    { background:#5a5f63; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
*[slabelOperationState="success"] { background:#2fa84f; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
*[slabelOperationState="failure"] { background:#c94f4f; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
```

- [ ] **Step 8: 构建并运行，确认 `test_font_tokens` 全部通过**

Run:
```bash
cmake --build build -j"$(nproc)"
ctest --test-dir build -R test_font_tokens --output-on-failure
```
Expected: 两个用例都 PASS。

- [ ] **Step 9: 跑全量回归测试（包含 `test_theme_files.cpp` 的既有断言）**

Run:
```bash
ctest --test-dir build --output-on-failure
```
Expected: 全部测试 PASS，尤其确认 `test_theme_files` 里 `defaultThemesContainFontAndIconTokens`（检查 `font-family` 字样仍存在）和 `defaultThemesContainOperationStateSelectors` 不受影响。

- [ ] **Step 10: Commit**

```bash
git add themes/default.qss themes/dark.qss themes/light.qss tests/test_font_tokens.cpp tests/test_font_tokens.pro tests/CMakeLists.txt tests/tests.pro
git commit -m "feat: 主题 QSS 按角色补充独立字号 token，初始值等于原 @fontSizeBase（零视觉变化）"
```

---

## Self-Review

**Spec coverage：**
- 架构决策 §1（`SControlEngine` 抽取，`SControl<Base>` 转发，`retranslate`/`HasSetText` 不下沉，`SControlEngine` 不实现 `ISControl`/不自注册，定时器改挂 `m_core`）→ Task 1。
- 架构决策 §2（角色字号 token，初始值=原 `fontSizeBase`，`* {}` 兜底保留）→ Task 3。
- `SControlBridge` 组合式用法 + `slabelAttach` 自由函数两种使用模式 → Task 2（`BridgedLabel` 测试类覆盖组合式；`attachReturnsNonNullAndAutoDestroysWithWidget` 覆盖 `slabelAttach`）。
- 文件清单表 11 行 → 全部有对应任务：`SControlEngine.h/.cpp`（Task1）、`SControlBridge.h/.cpp`（Task2）、`SControl.h` 转发改造（Task1）、三份主题 QSS（Task3）、`CMakeLists.txt`（Task1+Task2）、两个新测试文件+`.pro`（Task2/Task3）、`tests/CMakeLists.txt`（Task2+Task3）、`tests/tests.pro`（Task2+Task3）。
- 测试计划 6 点（组合式重译、事件过滤器重译、主题覆盖、操作状态机冒烟、`slabelAttach` 自动析构、既有回归套件不变）→ 全部对应 `test_control_bridge.cpp` 的 5 个用例 + Global Constraints 里列出的回归测试清单。
- 测试计划字号 2 点（QSS 角色规则用独立 token、`setFontSizePx` 生效）→ `test_font_tokens.cpp` 的 2 个用例。
- "不在本次范围内" 5 点均未在任务中出现新增内容，符合约束。

**Placeholder scan：** 全部代码块为完整代码，无 TBD/待补，无"同 Task N"式引用。

**Type consistency：** `SControlEngine`/`SControl<Base>`/`SControlBridge` 三处的 `setThemeOverride(const QString&, const QString&)`、`setFontSizePx(int)`、`setOperationHandler(std::function<void()>)`、`reportOperationResult(bool)`、`operationState() const -> OperationState`、`resetOperationState()`、`setOperationTimeoutMs(int)`/`setOperationResetDelayMs(int)`、`triggerOperation()`、`core() -> SControlCore&` 签名逐一核对一致。

---

**Plan complete and saved to `docs/superpowers/plans/2026-07-23-control-bridge-and-font-tokens-plan.md`. Two execution options:**

**1. Subagent-Driven (recommended)** - 每个任务派一个全新子代理执行，任务间做两阶段 review，迭代快

**2. Inline Execution** - 在当前会话里按批次执行任务，每批设检查点

**Which approach？**
