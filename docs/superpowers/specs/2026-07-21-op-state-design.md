# 控件操作状态反馈能力（RX_OP_STATE）设计

## 背景

需求规格说明（`需求规格说明.txt` 3.1.5 控件操作状态反馈能力 / 3.2.6 操作状态管理接口）新增了一项能力：具备"触发操作"语义的控件（如按钮）在被触发后，应依次呈现"待命/执行中/成功/失败"四种状态，并给出默认视觉反馈；超时未回报结果时自动判失败；状态在给定时间后或经手动复位自动回到待命。本设计描述该能力在现有 `slabel` 代码库中的落地方式。

## 架构决策

沿用项目现有模式：`SControl<Base>` 已经用同一套 CRTP 挂钩承载主题（`ThemeManager`）、语言（`LanguageManager`）能力；本能力不需要跨控件广播或两端撮合（不同于 Theme/Binding 的单例职责），因此直接在 `SControl<Base>` 上新增状态机相关的成员与方法，不引入新的单例管理器，也不引入多继承 mixin。

具体控件（首先是 `SButton`）只需把自己的"触发"信号接到 `SControl` 提供的 `protected` 方法 `triggerOperation()` 上。未来任何自定义控件（对应 3.1.6）接入方式完全一致：连一行信号即可获得完整状态机、超时、复位能力；只有默认视觉反馈需要控件作者自行在主题 QSS 里补选择器（见"视觉反馈"一节的范围说明）。

## 组件与 API

### 新增文件 `include/slabel/OperationState.h`

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

### `SControlCore`（`include/slabel/SControlCore.h`）新增信号

```cpp
signals:
    void bindablePropertyChanged(); // 既有：数据绑定用
    void operationStateChanged();   // 新增：操作状态切换时发出，不与前者混用
```

### `SControl<Base>`（`include/slabel/SControl.h`）新增公开接口

```cpp
void setOperationHandler(std::function<void()> handler);  // 触发登记：登记被触发时执行的处理方式
void reportOperationResult(bool success);                 // 结果回传：处理方式执行结束后告知结果
OperationState operationState() const;                    // 状态查询
void resetOperationState();                               // 复位：仅在 Success/Failure 时生效
void setOperationTimeoutMs(int ms);       // 执行中超时未回报 -> 自动切 Failure，默认 10000ms
void setOperationResetDelayMs(int ms);    // Success/Failure 自动回到 Idle 的等待时间，默认 2000ms

protected:
void triggerOperation();  // 具体控件把自己的"触发"信号接到这里
```

内部新增成员：`OperationState m_opState`、`std::function<void()> m_opHandler`、`QTimer m_opBusyTimer{this}`、`QTimer m_opResetTimer{this}`、`int m_opTimeoutMs`、`int m_opResetDelayMs`。两个 `QTimer` 是值成员，以 `this` 为 parent（`Base` 是 `QWidget`/`QObject`）通过花括号初始化构造，`timeout()` 连接到 `m_core`（已有 `Q_OBJECT`）作为 context object，析构时自动断开——延续 `BindingEngine` 已验证过的"借用已有 QObject 做 context"手法，不需要额外生命周期管理。

### 状态转移规则

对应需求 3.1.5.3（行为过程）与 3.1.5.4（行为参数：时序、异常条件及异常下的行为）：

1. `triggerOperation()`：若当前为 Busy，直接返回（重复触发不产生新操作，不异常）；若尚未通过 `setOperationHandler()` 登记任何处理方式，同样直接返回，不进入 Busy、不启动超时定时器——这是有意的设计决策：`SButton` 在构造函数里无条件把 `clicked()` 接到 `triggerOperation()`，若不做此判断，任何未接入操作能力、仅作普通按钮使用的 `SButton`（如 `examples/Gallery.cpp` 的 `themeBtn`/`langBtn`，只有外部 `connect` 的主题/语言切换 lambda，从未调用 `setOperationHandler`）每次点击都会被动进入状态机，产生无意义的 Busy→Failure 视觉抖动；只有在满足以上两条之后，才立即切到 Busy、按 `m_opTimeoutMs` 启动超时定时器，再同步调用登记的 `m_opHandler`。"立即切 Busy" 在调用 handler 之前完成，满足"不等待实际业务处理开始，第一时间给出反馈"。
2. `reportOperationResult(success)`：仅在当前为 Busy 时生效（防御性忽略非 Busy 时的迟到/多余回报，避免状态错乱，例如已超时判失败后业务方才慢悠悠地回报成功）；停止超时定时器，切到 Success 或 Failure，再按 `m_opResetDelayMs` 启动"自动回待命"定时器。
3. 超时定时器触发：若仍处 Busy，自动切 Failure，同样启动"自动回待命"定时器（对应异常条件三）。
4. "自动回待命"定时器触发：仅当前处 Success/Failure 才切回 Idle。
5. `resetOperationState()`：仅在 Success/Failure 时生效，提前停止"自动回待命"定时器并切回 Idle；Busy/Idle 时调用是 no-op。
6. 控件在 Busy 状态下被销毁：`~SControl()` 走既有清理路径（`QTimer` 作为子对象随对象树自动清理，`timeout` 连接以 `m_core` 为 context，`m_core` 作为成员随对象一起析构），不会有悬挂回调触发（对应异常条件二）。

### `SButton`（`include/slabel/SButton.h`）改动

把现有 `using SControl<QPushButton>::SControl;` 换成一个显式转发构造函数（保留原 `SControl` 构造函数的 SFINAE 保护逻辑，构造行为对现有调用方不变），构造体内追加：

```cpp
connect(this, &QPushButton::clicked, this, &SButton::triggerOperation);
```

其余具备"触发操作"语义的自定义控件接入方式与此完全一致（对应 3.1.6）。当前 15 个内置控件中，只有 `SButton` 具备"点击即触发一次性操作"的语义，其余（`SCheckBox`/`SRadioButton` 等）是持续性的状态型控件，本次不接入。

## 视觉反馈

状态切换时，`SControl` 把当前状态写成 Qt 动态属性 `slabelOperationState`（取值 `"busy"`/`"success"`/`"failure"`；Idle 时清空该属性），然后调用 `style()->unpolish(this)` / `style()->polish(this)` 强制刷新，复用现有的 QSS 变量替换与主题切换机制。

`themes/default.qss`、`themes/dark.qss` 各自追加 `SButton` 的状态选择器（具体色值在实现阶段确定，风格与现有 `@primary` 等 token 一致）：

```css
SButton[slabelOperationState="busy"]    { ... }
SButton[slabelOperationState="success"] { ... }
SButton[slabelOperationState="failure"] { ... }
```

**范围说明**：内置默认视觉反馈只对 slabel 自带控件（当前即 `SButton`）通过随库分发的主题 QSS 提供。自定义控件（3.1.6）通过连接 `triggerOperation()` 免费获得完整的状态机/超时/复位能力，但默认视觉反馈需要控件作者自行在其主题 QSS 中补充对应的属性选择器；若未补充，控件在各状态下维持自身原有外观、不报错——这是有意的降级行为，避免把"任意 QWidget 的默认状态绘制"做成本次范围（YAGNI）。`setThemeOverride`/`clearThemeOverride` 仍按既有语义工作，不针对单个操作状态做特殊覆盖 API。

## 测试计划

新增 `tests/test_opstate.cpp`（沿用项目 `QT_QPA_PLATFORM=offscreen` + Qt Test 约定），覆盖：

1. 触发后立即变 Busy，登记的 handler 被调用一次；
2. Busy 期间重复触发——handler 调用次数不变（no-op 验证，对应异常条件一）；
3. `reportOperationResult(true)`/`(false)` 后变 Success/Failure，配置一个短的 reset-delay（如 50ms）后 `QTRY_COMPARE` 自动回 Idle；
4. 不回报结果，配置一个短超时（如 50ms），到时自动变 Failure，并进入下一轮自动回待命；
5. `resetOperationState()` 在 Success/Failure 时提前生效；在 Busy/Idle 时调用是 no-op；
6. Busy 状态下销毁控件（heap 分配后 `delete`）不崩溃（对应异常条件二）；
7. `SButton` 点击（`QTest::mouseClick` 或直接 `emit clicked()`）确实自动触发状态机（集成验证，验证 3.1.1 到 3.1.5 的联动）。

`tests/CMakeLists.txt` 按现有模式登记新测试文件。

## 不在本次范围内

- 不为 Busy 状态提供"取消"接口（需求未要求，仅超时自动判失败）。
- 不为 `SCheckBox`/`SRadioButton` 等非一次性触发语义的控件接入此能力。
- 不提供针对单个操作状态的专属样式覆盖 API（复用 3.1.2 已有的 `setThemeOverride` 机制即可满足"可定制"的需求文字）。
- 不为未随库分发主题 QSS 的自定义控件生成兜底视觉效果。
