# 控件操作状态反馈能力（RX_OP_STATE）Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为具备"触发操作"语义的控件（首先是 `SButton`）实现待命/执行中/成功/失败四态操作状态反馈能力，含默认视觉反馈与可配置超时/复位。

**Architecture:** 在既有 `SControl<Base>` CRTP 能力模板上直接新增状态机相关成员与方法（不引入新单例、不引入多继承 mixin），具体控件只需把自己的"触发"信号接到 `SControl` 提供的 `protected triggerOperation()` 上即可接入；默认视觉反馈复用现有 QSS 主题机制（Qt 动态属性 + 属性选择器）。

**Tech Stack:** Qt 5.15 / C++17，`QTimer`、Qt 动态属性 + `QStyle::polish`、Qt Test（`QT_QPA_PLATFORM=offscreen`）。

## Global Constraints

- Qt 5.15.2/5.15.10，C++17，仅动态库产物（`SLABEL_EXPORT`/`SLABEL_BUILD` 宏保持现状）。
- CMake（`file(GLOB CONFIGURE_DEPENDS ...)`）与 qmake（`$$files(...)`）双构建均需保持可用；新增 `.h` 放入 `include/slabel/`、新增 `.cpp` 放入 `src/` 即可被两套构建自动纳入，无需改动顶层 `CMakeLists.txt`/`standard_lable.pro`。
- 测试统一用 Qt Test，通过 `tests/CMakeLists.txt` 里的 `slabel_test(name)` 函数注册，运行时环境变量 `QT_QPA_PLATFORM=offscreen`（该函数已自动设置）。
- 代码注释与文档使用简体中文（对话可用繁体，但产出文件用简体——这是本仓库既有约定）。
- 遵循 TDD：每个行为先写失败测试，再写最小实现使其通过。
- 状态机/超时相关的默认值：执行中超时默认 `10000`ms，成功/失败自动回待命延时默认 `2000`ms（均可通过 setter 覆盖，本计划的测试里为了跑得快会设置更短的值）。
- 状态转移规则（详见设计文档 `docs/superpowers/specs/2026-07-21-op-state-design.md`）：
  - `triggerOperation()` 在 Busy 时重复调用是 no-op；否则立即转 Busy、启动超时定时器，再同步调用登记的 handler。
  - `reportOperationResult(success)` 仅在 Busy 时生效，转 Success/Failure 并启动"自动回待命"定时器。
  - 超时定时器触发：仍处 Busy 才转 Failure。
  - "自动回待命"定时器触发：仍处 Success/Failure 才转 Idle。
  - `resetOperationState()` 仅在 Success/Failure 时生效，提前转 Idle。

---

### Task 1: OperationState 状态机核心（SControl + SButton 接线）

**Files:**
- Create: `include/slabel/OperationState.h`
- Modify: `include/slabel/SControlCore.h`
- Modify: `include/slabel/SControl.h`
- Modify: `include/slabel/SButton.h`
- Create: `tests/test_opstate.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `enum class OperationState { Idle, Busy, Success, Failure };`（全局作用域，`include/slabel/OperationState.h`）。
- Produces（`SControl<Base>` 公开方法，供 Task 2 与后续控件使用）：
  - `void setOperationHandler(std::function<void()> handler)`
  - `void reportOperationResult(bool success)`
  - `OperationState operationState() const`
  - `void resetOperationState()`
  - `void setOperationTimeoutMs(int ms)`
  - `void setOperationResetDelayMs(int ms)`
  - `SControlCore& core()`（暴露 `operationStateChanged()` 信号）
  - `protected: void triggerOperation()`（具体控件把自己的触发信号接到这里）
- Produces（`SControlCore` 新增信号）：`void operationStateChanged();`

- [ ] **Step 1: 写新增头文件 `include/slabel/OperationState.h`**

```cpp
#pragma once

// 控件操作状态：具备"触发操作"语义的控件（如按钮）在一次操作生命周期内所处的阶段
enum class OperationState {
    Idle,     // 待命
    Busy,     // 执行中
    Success,  // 成功
    Failure   // 失败
};
```

- [ ] **Step 2: 给 `SControlCore` 加新信号**

打开 `include/slabel/SControlCore.h`，把内容改为：

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
    void notifyOperationStateChanged() { emit operationStateChanged(); }
signals:
    void bindablePropertyChanged();
    void operationStateChanged();
};
```

- [ ] **Step 3: 写失败测试 `tests/test_opstate.cpp`**

```cpp
#include <QtTest>
#include "slabel/SButton.h"

class TestOpState : public QObject {
    Q_OBJECT
private slots:
    void triggerEntersBusyAndCallsHandlerOnce() {
        SButton btn;
        int calls = 0;
        btn.setOperationHandler([&]{ ++calls; });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        QCOMPARE(calls, 1);
    }
    void repeatedTriggerWhileBusyIsNoOp() {
        SButton btn;
        int calls = 0;
        btn.setOperationHandler([&]{ ++calls; });
        btn.click();
        btn.click();
        btn.click();
        QCOMPARE(calls, 1);
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
    }
    void successThenAutoRevertsToIdle() {
        SButton btn;
        btn.setOperationResetDelayMs(30);
        btn.setOperationHandler([&]{ btn.reportOperationResult(true); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Success));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void failureThenAutoRevertsToIdle() {
        SButton btn;
        btn.setOperationResetDelayMs(30);
        btn.setOperationHandler([&]{ btn.reportOperationResult(false); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Failure));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void reportResultIgnoredWhenNotBusy() {
        SButton btn;
        btn.reportOperationResult(true);  // Idle 时回报，应被忽略
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void timeoutWithoutReportBecomesFailure() {
        SButton btn;
        btn.setOperationTimeoutMs(30);
        btn.click(); // 不登记 handler，也不回报结果
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Failure));
    }
    void resetOperationStateEarlyFromFailure() {
        SButton btn;
        btn.setOperationResetDelayMs(10000); // 足够长，确保下面的复位不是自动回退触发的
        btn.setOperationHandler([&]{ btn.reportOperationResult(false); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Failure));
        btn.resetOperationState();
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void resetOperationStateIsNoOpWhenBusyOrIdle() {
        SButton btn;
        btn.resetOperationState(); // Idle 时 no-op
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
        btn.setOperationHandler([]{});
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        btn.resetOperationState(); // Busy 时 no-op
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
    }
    void destroyingBusyControlDoesNotCrash() {
        auto* btn = new SButton;
        btn->setOperationHandler([]{});
        btn->click();
        QCOMPARE(int(btn->operationState()), int(OperationState::Busy));
        delete btn; // 不应崩溃
    }
    void operationStateChangedSignalFiresOnEachTransition() {
        SButton btn;
        QSignalSpy spy(&btn.core(), &SControlCore::operationStateChanged);
        btn.setOperationHandler([&]{ btn.reportOperationResult(true); });
        btn.click();
        QCOMPARE(spy.count(), 2); // Idle->Busy, Busy->Success
    }
};

QTEST_MAIN(TestOpState)
#include "test_opstate.moc"
```

- [ ] **Step 4: 把新测试注册进 `tests/CMakeLists.txt`**

在现有 `slabel_test(test_theme_files)` 那行之后追加一行：

```cmake
slabel_test(test_opstate)
```

- [ ] **Step 5: 运行测试，确认因符号未定义而编译失败**

```bash
cmake --build build -j$(nproc) 2>&1 | tail -40
```

Expected: 编译失败，报 `OperationState`/`operationState`/`setOperationHandler` 等未定义（`SButton`/`SControl` 尚无这些成员）。

- [ ] **Step 6: 给 `SControl<Base>` 加状态机实现**

打开 `include/slabel/SControl.h`，按下面的完整新内容替换整个文件：

```cpp
#pragma once
#include <QWidget>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QTimer>
#include <QStyle>
#include <QCoreApplication>
#include <functional>
#include <type_traits>
#include <utility>
#include "slabel/ISControl.h"
#include "slabel/SControlCore.h"
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
// 注意：模板类不含 Q_OBJECT；signal 由成员 m_core 提供。
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
    explicit SControl(Args&&... args) : Base(std::forward<Args>(args)...) {
        ThemeManager::instance().registerControl(this);
        LanguageManager::instance().registerControl(this);
        m_opBusyTimer.setSingleShot(true);
        m_opResetTimer.setSingleShot(true);
        QObject::connect(&m_opBusyTimer, &QTimer::timeout, &m_core, [this]{ onOperationTimeout(); });
        QObject::connect(&m_opResetTimer, &QTimer::timeout, &m_core, [this]{ onOperationResetTimeout(); });
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

    // 主题：代码覆盖（widget 级 QSS，优先级高于应用级）
    void setThemeOverride(const QString& key, const QString& value) {
        m_overrides.insert(key, value);
        applyOverrides();
    }
    void clearThemeOverride() {
        m_overrides.clear();
        applyOverrides();
    }

    // 操作状态反馈能力（RX_OP_STATE）
    SControlCore& core() { return m_core; }  // 暴露 operationStateChanged 等信号

    void setOperationHandler(std::function<void()> handler) {
        m_opHandler = std::move(handler);
    }
    // 结果回传：仅在"执行中"时生效，迟到/多余的回报会被忽略
    void reportOperationResult(bool success) {
        if (m_opState != OperationState::Busy) return;
        m_opBusyTimer.stop();
        setOpState(success ? OperationState::Success : OperationState::Failure);
        if (m_opResetDelayMs > 0) m_opResetTimer.start(m_opResetDelayMs);
    }
    OperationState operationState() const { return m_opState; }
    // 复位：仅在"成功"/"失败"时生效，提前恢复为"待命"
    void resetOperationState() {
        if (m_opState != OperationState::Success && m_opState != OperationState::Failure) return;
        m_opResetTimer.stop();
        setOpState(OperationState::Idle);
    }
    void setOperationTimeoutMs(int ms) { m_opTimeoutMs = ms; }
    void setOperationResetDelayMs(int ms) { m_opResetDelayMs = ms; }

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

    // 具体控件（或自定义控件）把自己的"触发"信号接到这里即可接入操作状态反馈能力
    void triggerOperation() {
        if (m_opState == OperationState::Busy) return; // 重复触发不产生新操作
        setOpState(OperationState::Busy);
        if (m_opTimeoutMs > 0) m_opBusyTimer.start(m_opTimeoutMs);
        if (m_opHandler) m_opHandler();
    }

    SControlCore m_core;
    QHash<QString, QString> m_overrides;
    QByteArray m_sourceText;

private:
    void setOpState(OperationState s) {
        m_opState = s;
        applyOperationVisual();
        m_core.notifyOperationStateChanged();
    }
    // 默认视觉反馈：写入 Qt 动态属性，交由主题 QSS 的属性选择器呈现
    void applyOperationVisual() {
        QWidget* w = this;
        switch (m_opState) {
            case OperationState::Busy:    w->setProperty("slabelOperationState", QStringLiteral("busy")); break;
            case OperationState::Success: w->setProperty("slabelOperationState", QStringLiteral("success")); break;
            case OperationState::Failure: w->setProperty("slabelOperationState", QStringLiteral("failure")); break;
            default:                      w->setProperty("slabelOperationState", QVariant()); break;
        }
        w->style()->unpolish(w);
        w->style()->polish(w);
        w->update();
    }
    void onOperationTimeout() {
        if (m_opState == OperationState::Busy) {
            setOpState(OperationState::Failure);
            if (m_opResetDelayMs > 0) m_opResetTimer.start(m_opResetDelayMs);
        }
    }
    void onOperationResetTimeout() {
        if (m_opState == OperationState::Success || m_opState == OperationState::Failure)
            setOpState(OperationState::Idle);
    }

    OperationState m_opState = OperationState::Idle;
    std::function<void()> m_opHandler;
    QTimer m_opBusyTimer{this};
    QTimer m_opResetTimer{this};
    int m_opTimeoutMs = 10000;
    int m_opResetDelayMs = 2000;
};
```

- [ ] **Step 7: 把 `SButton` 接到 `triggerOperation()`**

把 `include/slabel/SButton.h` 整个替换为：

```cpp
#pragma once
#include <QPushButton>
#include <QEvent>
#include <type_traits>
#include <utility>
#include "slabel/SGlobal.h"
#include "slabel/SControl.h"

class SLABEL_EXPORT SButton : public SControl<QPushButton> {
    Q_OBJECT
public:
    // 转发构造 + 接入操作状态反馈能力：把 clicked() 接到 triggerOperation()。
    // SFINAE 排除单参数且为 SButton 派生/同类的情况，避免劫持拷贝/移动构造。
    template<typename... Args,
             typename = std::enable_if_t<
                 !(sizeof...(Args) == 1 &&
                   (std::is_base_of_v<SButton, std::decay_t<Args>> || ...))>>
    explicit SButton(Args&&... args)
        : SControl<QPushButton>(std::forward<Args>(args)...) {
        connect(this, &QPushButton::clicked, this, &SButton::triggerOperation);
    }
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange)
            retranslate();
        QPushButton::changeEvent(e);
    }
};
```

- [ ] **Step 8: 编译并跑测试，确认全部通过**

```bash
cmake --build build -j$(nproc) && QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure -R test_opstate
```

Expected: `test_opstate` 全部用例 PASS。

- [ ] **Step 9: 跑全量测试，确认没有破坏既有功能**

```bash
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

Expected: 全部测试 PASS（在 Task 1 之前应为 7 个可执行测试，加上本任务新增的 `test_opstate` 共 8 个）。

- [ ] **Step 10: Commit**

```bash
git add include/slabel/OperationState.h include/slabel/SControlCore.h \
        include/slabel/SControl.h include/slabel/SButton.h \
        tests/test_opstate.cpp tests/CMakeLists.txt
git commit -m "feat: 控件操作状态反馈能力(RX_OP_STATE)状态机核心 + SButton 接入"
```

---

### Task 2: 默认视觉反馈（主题 QSS）

**Files:**
- Modify: `themes/default.qss`
- Modify: `themes/dark.qss`
- Modify: `tests/test_theme_files.cpp`

**Interfaces:**
- Consumes: Task 1 产出的动态属性 `slabelOperationState`（取值 `"busy"`/`"success"`/`"failure"`，`Idle` 时属性为空）。
- Produces: 无新代码接口——本任务只新增主题 QSS 内容与对应的文件内容校验测试。

- [ ] **Step 1: 写失败测试，校验默认主题文件包含状态选择器**

打开 `tests/test_theme_files.cpp`，在 `unknownThemeFails` 后面追加一个新测试槽：

```cpp
    void defaultThemesContainOperationStateSelectors() {
        QFile def(QStringLiteral(THEME_DIR "/default.qss"));
        QVERIFY(def.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString defText = QString::fromUtf8(def.readAll());
        QVERIFY(defText.contains(QStringLiteral("SButton[slabelOperationState=\"busy\"]")));
        QVERIFY(defText.contains(QStringLiteral("SButton[slabelOperationState=\"success\"]")));
        QVERIFY(defText.contains(QStringLiteral("SButton[slabelOperationState=\"failure\"]")));

        QFile dark(QStringLiteral(THEME_DIR "/dark.qss"));
        QVERIFY(dark.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString darkText = QString::fromUtf8(dark.readAll());
        QVERIFY(darkText.contains(QStringLiteral("SButton[slabelOperationState=\"busy\"]")));
        QVERIFY(darkText.contains(QStringLiteral("SButton[slabelOperationState=\"success\"]")));
        QVERIFY(darkText.contains(QStringLiteral("SButton[slabelOperationState=\"failure\"]")));
    }
```

并在文件顶部的 `#include <QtTest>` 后面加一行 `#include <QFile>`。

- [ ] **Step 2: 运行测试，确认失败（当前主题文件里还没有这些选择器）**

```bash
cmake --build build -j$(nproc) && QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure -R test_theme_files
```

Expected: `defaultThemesContainOperationStateSelectors` FAIL。

- [ ] **Step 3: 给 `themes/default.qss` 追加状态选择器**

在文件末尾追加：

```css
SButton[slabelOperationState="busy"]    { background:#6c757d; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; }
SButton[slabelOperationState="success"] { background:#28a745; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; }
SButton[slabelOperationState="failure"] { background:#dc3545; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; }
```

- [ ] **Step 4: 给 `themes/dark.qss` 追加状态选择器**

在文件末尾追加：

```css
SButton[slabelOperationState="busy"]    { background:#5a5f63; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
SButton[slabelOperationState="success"] { background:#2fa84f; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
SButton[slabelOperationState="failure"] { background:#c94f4f; color:#eff0f1; border:none; border-radius:4px; padding:6px 12px; }
```

- [ ] **Step 5: 重新构建，确认测试通过**

```bash
cmake --build build -j$(nproc) && QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure -R test_theme_files
```

Expected: `test_theme_files` 全部用例 PASS。

- [ ] **Step 6: 跑全量测试**

```bash
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

Expected: 全部 PASS。

- [ ] **Step 7: Commit**

```bash
git add themes/default.qss themes/dark.qss tests/test_theme_files.cpp
git commit -m "feat: 为 SButton 操作状态补充默认视觉反馈的主题 QSS 规则"
```

---

### Task 3: 收尾验证与文档

**Files:**
- Modify: `.superpowers/sdd/progress.md`
- Modify: `docs/superpowers/specs/2026-07-20-standard-lable-design.md`

**Interfaces:**
- Consumes: Task 1、Task 2 的全部产出（无新代码接口）。

- [ ] **Step 1: 更新原设计文档，补一句指向新能力**

打开 `docs/superpowers/specs/2026-07-20-standard-lable-design.md`，在描述现有控件能力清单的章节末尾（控件/主题/语言/绑定能力介绍之后）追加一段：

```markdown
### 操作状态反馈能力

`SButton`（及未来任何具备"触发操作"语义的控件）通过 `SControl<Base>` 提供的
`setOperationHandler`/`reportOperationResult`/`operationState`/
`resetOperationState`/`setOperationTimeoutMs`/`setOperationResetDelayMs` 接入
待命/执行中/成功/失败四态状态机，默认视觉反馈通过 Qt 动态属性
`slabelOperationState` + 主题 QSS 属性选择器呈现。详见
`docs/superpowers/specs/2026-07-21-op-state-design.md`。
```

- [ ] **Step 2: 更新 SDD 进度帐本**

打开 `.superpowers/sdd/progress.md`，在文件末尾追加：

```markdown

Task 11(RX_OP_STATE 状态机核心): complete —— OperationState 枚举 + SControlCore
  新信号 + SControl 状态机(触发/回报/查询/复位/超时/自动回待命) + SButton 接入，
  tests/test_opstate.cpp 覆盖全部转移场景，全量测试通过。
Task 12(RX_OP_STATE 默认视觉反馈): complete —— default.qss/dark.qss 补
  slabelOperationState 属性选择器，test_theme_files.cpp 校验文件内容。
```

- [ ] **Step 3: 跑一次全量测试 + qmake 编译校验**

```bash
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
mkdir -p /tmp/slabel-qmake-check && cd /tmp/slabel-qmake-check \
  && qmake /opt/xw/standard_lable/standard_lable.pro && make -j$(nproc) \
  && ls libslabel.so* && cd /opt/xw/standard_lable
```

Expected: ctest 全部 PASS；qmake 构建产出 `libslabel.so*`。

- [ ] **Step 4: Commit**

```bash
git add .superpowers/sdd/progress.md docs/superpowers/specs/2026-07-20-standard-lable-design.md
git commit -m "docs: 记录 RX_OP_STATE 状态机/视觉反馈的 SDD 进度与设计文档回链"
```
