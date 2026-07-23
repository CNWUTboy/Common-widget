# 既有控件能力桥接（SControlBridge）与字号角色化设计

## 背景

自测中发现现有 `SControl<Base>`（`include/slabel/SControl.h`）机制的两个问题：

1. `SControl<Base>` 是编译期 CRTP 混入模板，主题/语言/操作状态等能力与控件基类强绑定，只有在类声明时就写成 `class X : public SControl<QWidget>` 才能获得。业务代码里已经写好的自定义控件类（`class X : public QWidget {...}`）无法在不改动基类声明的前提下接入这些能力。
2. 三个主题 QSS（`themes/default.qss`、`themes/dark.qss`、`themes/light.qss`）都用 `* { font-family:@fontFamily; font-size:@fontSizeBase; }` 一条通配规则同时控制字体家族与字号。字体家族统一是合理诉求，但字号被同一条规则、同一个 token 锁死，没有任何角色规则或实例真正覆盖过它，导致所有控件字号事实上被统一控制。

本设计解决这两个问题，都是在现有机制上做扩展，不引入新的单例管理器，不改变已发布的公开 API 行为。

## 架构决策

### 1. 抽出共享引擎 `SControlEngine`，`SControlBridge` 与 `SControl<Base>` 共用

`SControl<Base>` 目前把"文案重译、主题覆盖表、操作状态机（含两个 QTimer）"这三块逻辑直接写在模板体内。**只把与 `Base` 具体类型无关的两块——主题覆盖表、操作状态机——抽成共享类** `SControlEngine`（`include/slabel/SControlEngine.h` + `src/SControlEngine.cpp`），构造时传入目标 `QWidget*`，`SControl<Base>` 和新增的 `SControlBridge` 都持有一个 `SControlEngine` 成员并把这两块的公开方法转发给它。

**文案重译（`setTextTr`/`retranslate`）不抽进共享引擎，`SControl<Base>` 保留原有的编译期 `HasSetText<Base>` SFINAE 实现原封不动**——这是为了不影响现有 15 个内置控件已经在用、经过验证的零开销路径（直接调用 `Base::setText()`，无需经过 `QMetaObject` 属性查找）。`SControlBridge` 拿不到 `Base` 的编译期类型信息，只能自己另外实现一份**运行时**版本：检查 `target->metaObject()->indexOfProperty("text") >= 0`，存在则 `target->setProperty("text", translated)`（`QLabel`/`QAbstractButton`/`QLineEdit` 等都有 `text` 的 `Q_PROPERTY`，`setProperty` 经由该属性的 WRITE 函数间接调用到 `setText()`，效果等价；没有该属性的控件如 `QComboBox`/`QSpinBox` 自动跳过）。这份运行时实现只在 `SControlBridge` 内部使用，不影响 `SControl<Base>` 的现有性能与行为；`include/slabel/SControl.h` 里的 `slabel_detail::HasSetText` 保留不删。

`SControlEngine` **不**实现 `ISControl`、不负责向 `ThemeManager`/`LanguageManager` 注册——注册动作和 `asWidget()`/`retranslate()` 的对外契约仍由持有者（`SControl<Base>` 或 `SControlBridge`）自己完成，原因：
- `SControl<Base>` 现有测试断言 `btn.asWidget() == &btn`（`tests/test_scontrol_theme.cpp`、`tests/test_controls.cpp`），`asWidget()` 必须返回 `SControl<Base>` 自身，不能是内部引擎，注册身份不能换。
- 如果 `SControlEngine` 自己也注册一份，会导致语言切换时 `LanguageManager` 对同一个控件调用两次 `retranslate()`（一次通过持有者，一次通过引擎）——重复但表面无害，属于不必要的浪费，不做。

`SControl<Base>` 现有的 `changeEvent` 覆写（响应 `QEvent::LanguageChange`，见 `tests/test_language.cpp` 的 `changeEventDispatchesToRetranslate` 用例）**保留不变**，仍然直接调用自己的 `retranslate()`（编译期路径）。`SControlBridge` 不是 `QWidget`/`QObject` 的天然事件目标，用 `target->installEventFilter(this)` 在自己的 `eventFilter()` 里捕获同一事件，触发运行时版本的 `retranslate()`，达到同等效果。

**性能与兼容性小结**：现有 `SControl<Base>` 及基于它的 15 个内置控件——文案重译路径不变（仍是编译期直接调用），主题覆盖表和操作状态机只是把已有代码原样搬到 `m_engine` 成员里、操作对象从 `this` 换成传入的 `QWidget*`，指令序列不变，不新增任何调用开销；公开 API 签名和行为完全不变，业务代码零改动。新增的运行时属性查找、`installEventFilter` 事件拦截（`eventFilter` 会收到目标 widget 的所有事件，不止 `LanguageChange`，多一次虚函数转发）只发生在**新增的** `SControlBridge` 路径上，是"给已有自定义控件接入能力"这个新功能本身必须付出的代价，不触及现有代码。

### 2. 字号从"一条通配规则"变成"每个角色一个独立 token"

不改变级联优先级机制本身（Qt QSS 里 `[slabelRole=...]` 天然比 `*` specificity 高，控件自身 `setStyleSheet` 天然比 app 级样式表优先，这两条现状已经成立，见 `SControl.h` 里 `applyOverrides()` 的注释），只改 QSS 内容：给每个既有 `[slabelRole="..."]` 规则块补一条 `font-size`，值来自该角色**独立的** token（初始值等于 `@fontSizeBase`，视觉效果与现状完全一致），而不是继续依赖 `*` 兜底。没有分配角色、也没有实例覆盖的控件，仍然落到 `*` 的 `@fontSizeBase` 兜底——这一层保留，因为它是"没人特意设置时给个合理默认值"，不是"强制统一"。

不在这次设计里为各角色发明具体的像素值（例如"标题应该多大"），那是主题作者按需调整的事；本次只保证每个角色有自己独立可调的 token。

## 组件与 API

### 新增/改动文件清单

| 文件 | 改动 |
|---|---|
| `include/slabel/SControlEngine.h`（新增） | 共享引擎，见下 |
| `src/SControlEngine.cpp`（新增） | 引擎实现 |
| `include/slabel/SControlBridge.h`（新增） | 组合式桥接 + `slabelAttach()` 自由函数 |
| `src/SControlBridge.cpp`（新增） | 桥接实现 |
| `include/slabel/SControl.h` | 覆盖表/操作状态机改为转发给内部 `SControlEngine` 成员；`retranslate`/`setTextTr`/`HasSetText` SFINAE 保留原样不动 |
| `themes/default.qss` / `dark.qss` / `light.qss` | 各角色规则补充独立 `font-size` token |
| `CMakeLists.txt` | `SLABEL_SOURCES`/`SLABEL_HEADERS` 追加新文件（`standard_lable.pro` 用 `$$files()` 通配 `include/slabel/*.h`、`src/*.cpp`，新文件放对目录即自动纳入，不用改） |
| `tests/test_control_bridge.cpp` + `test_control_bridge.pro`（新增） | 桥接能力测试 |
| `tests/test_font_tokens.cpp` + `test_font_tokens.pro`（新增） | 字号级联优先级测试 |
| `tests/CMakeLists.txt` | 追加两个 `slabel_test(...)` 调用 |
| `tests/tests.pro` | `SUBDIRS` 追加两个新测试目标 |

### `SControlEngine`

```cpp
// include/slabel/SControlEngine.h
class SLABEL_EXPORT SControlEngine {
public:
    explicit SControlEngine(QWidget* widget);
    ~SControlEngine();

    void setThemeOverride(const QString& key, const QString& value);
    void clearThemeOverride();
    void setFontSizePx(int px);  // 便捷方法：setThemeOverride("font-size", "%1px")

    SControlCore& core();

    void setOperationHandler(std::function<void()> handler);
    void reportOperationResult(bool success);
    OperationState operationState() const;
    void resetOperationState();
    void setOperationTimeoutMs(int ms);
    void setOperationResetDelayMs(int ms);
    void triggerOperation();

private:
    QWidget* m_widget;  // 非持有，生命周期由调用方保证
    SControlCore m_core;
    QHash<QString, QString> m_overrides;
    OperationState m_opState = OperationState::Idle;
    std::function<void()> m_opHandler;
    QTimer m_opBusyTimer;
    QTimer m_opResetTimer;
    int m_opTimeoutMs = 10000;
    int m_opResetDelayMs = 2000;
    // applyOverrides()/setOpState()/applyOperationVisual()/onOperationTimeout()/
    // onOperationResetTimeout() 原样从 SControl.h 搬入，操作对象从 this 换成 m_widget
};
```

不含 `setTextTr`/`retranslate`/`m_sourceText`——这两个方法留在各自持有者里单独实现（见上一节说明）。`m_widget` 只是借用指针，`SControlEngine` 不管理其生命周期——持有者（`SControl<Base>` 用 `this`，`SControlBridge` 用外部传入的指针）负责保证引擎不会 outlive 目标控件。两个 `QTimer` 成员构造时不再以 `this`（非 `QObject`）为 parent，改为以 `m_core`（已是 `QObject`）为 parent，`timeout` 信号同样连到 `m_core` 作为 context object，语义与现状一致。

### `SControl<Base>` 改动

```cpp
template<class Base>
class SControl : public Base, public ISControl {
public:
    explicit SControl(Args&&... args)
        : Base(std::forward<Args>(args)...), m_engine(this) {
        ThemeManager::instance().registerControl(this);
        LanguageManager::instance().registerControl(this);
    }
    ~SControl() override {
        ThemeManager::instance().unregisterControl(this);
        LanguageManager::instance().unregisterControl(this);
    }

    QWidget* asWidget() override { return this; }

    // 与现状完全一致：编译期 HasSetText<Base> 分支，不经过 m_engine
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

    void setThemeOverride(const QString& k, const QString& v) { m_engine.setThemeOverride(k, v); }
    void clearThemeOverride() { m_engine.clearThemeOverride(); }
    void setFontSizePx(int px) { m_engine.setFontSizePx(px); }
    SControlCore& core() { return m_engine.core(); }
    void setOperationHandler(std::function<void()> h) { m_engine.setOperationHandler(std::move(h)); }
    void reportOperationResult(bool ok) { m_engine.reportOperationResult(ok); }
    OperationState operationState() const { return m_engine.operationState(); }
    void resetOperationState() { m_engine.resetOperationState(); }
    void setOperationTimeoutMs(int ms) { m_engine.setOperationTimeoutMs(ms); }
    void setOperationResetDelayMs(int ms) { m_engine.setOperationResetDelayMs(ms); }
    void triggerOperation() { m_engine.triggerOperation(); }

protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        Base::changeEvent(e);
    }

private:
    SControlEngine m_engine;
    QByteArray m_sourceText;
};
```

对外行为、方法签名、内部执行路径与现状完全一致，业务代码（`SLabel`/`SButton` 等 15 个内置控件、Gallery 示例）不需要任何改动，性能也不受影响——`retranslate`/`setTextTr`/`changeEvent` 三处都是把现状代码原样保留，只有 `setThemeOverride`/`clearThemeOverride`/操作状态机这几个方法体从"直接操作 `this`"变成"转发一层给 `m_engine`"。

### `SControlBridge`：组合成员 + 自由挂载函数

```cpp
// include/slabel/SControlBridge.h
class SLABEL_EXPORT SControlBridge : public QObject, public ISControl {
    Q_OBJECT
public:
    explicit SControlBridge(QWidget* target, QObject* parent = nullptr);
    ~SControlBridge() override;

    QWidget* asWidget() override { return m_target; }

    void setTextTr(const char* sourceText);  // 运行时版本：见上一节
    void retranslate() override;             // 按 "text" 属性探测，无该属性则 no-op

    // 其余方法（setThemeOverride/clearThemeOverride/setFontSizePx/操作状态机）
    // 与 SControlEngine 的公开接口一一转发，签名同上（略）

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    QWidget* m_target;
    SControlEngine m_engine;
    QByteArray m_sourceText;
};

// 自由挂载：以 widget 为 QObject parent，随其销毁自动释放；零源码改动即可接入
SLABEL_EXPORT SControlBridge* slabelAttach(QWidget* widget);
```

用法对应两个既定场景：

```cpp
// 场景一：组合成员（源码可改，加一行）
class MyWidget : public QWidget {
public:
    explicit MyWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setProperty("slabelRole", "myWidgetRole");  // 推荐：角色贴在类型自己的构造函数里，写一次
        m_slabel.setOperationHandler([this]{ /* ... */ });
    }
private:
    SControlBridge m_slabel{this};
};

// 场景二：自由挂载（零源码改动）
MyWidget* w = new MyWidget(parent);
auto* bridge = slabelAttach(w);
bridge->setOperationHandler([]{ /* ... */ });
```

场景一里 `SControlBridge` 作为值成员，`this`（`MyWidget*`）既是 `target` 又是 `QObject parent`——注意 `SControlBridge` 此时以 `MyWidget` 为 `QObject` 树上的 parent，析构顺序由 C++ 成员声明顺序保证（先于 `MyWidget` 自身析构，因为是它的成员），不依赖 Qt 的 parent-child 自动删除。场景二里 `slabelAttach` 在堆上 `new SControlBridge(widget, /*parent=*/widget)`，完全依赖 Qt parent-child 机制随 `widget` 销毁而自动释放。

**角色标记的推荐写法**：`slabelRole` 决定控件套用哪套集中定义在主题 QSS 里的视觉规则（颜色、字号等），现状（含 `SControl<Base>` 路径）都是调用方手动 `setProperty("slabelRole", ...)` 贴上去的，库本身不会替你自动贴。对于场景一这种源码可改的自定义控件类，建议把这行 `setProperty` 写在类自己的构造函数里（如上例），而不是留给每个实例化调用点——这样同一类型的所有实例自动带上一致的角色，不用每次 `new MyWidget(...)` 都记得再写一遍；这只是"这行代码写在哪"的位置选择，调用次数不变（每个实例仍然只执行一次），没有额外性能开销。场景二（零源码改动）没有构造函数可插入，角色标记只能由挂载方在 `slabelAttach` 之后自行 `setProperty`。

### 主题 QSS 改动（示例，三个主题同构处理）

```diff
 /* @primary:#0d6efd @bg:#ffffff @text:#212529 @border:#ced4da
    @fontFamily:"Segoe UI" @fontSizeBase:13px
+   @fontSizeButton:13px @fontSizeInput:13px @fontSizeCombo:13px
+   @fontSizeCheckBox:13px @fontSizeRadioButton:13px @fontSizeGroupBox:13px @fontSizeProgressBar:13px
    @iconSave:":/icons/light/save.svg" */

 * { font-family:@fontFamily; font-size:@fontSizeBase; }

-*[slabelRole="primaryButton"] { background:@primary; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; qproperty-icon: url(@iconSave); }
-*[slabelRole="textInput"]     { background:@bg; color:@text; border:1px solid @border; border-radius:4px; padding:4px; }
+*[slabelRole="primaryButton"] { background:@primary; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; font-size:@fontSizeButton; qproperty-icon: url(@iconSave); }
+*[slabelRole="textInput"]     { background:@bg; color:@text; border:1px solid @border; border-radius:4px; padding:4px; font-size:@fontSizeInput; }
 ... (comboBox/checkBox/radioButton/groupBox/progressBar 同样各补一个 font-size:@token)
```

`dark.qss`、`light.qss` 按各自现有 token 值同构处理（初始值沿用各主题原本的 `@fontSizeBase`，不引入视觉差异）。

## 测试计划

`tests/test_control_bridge.cpp`（新增，沿用项目 `QT_QPA_PLATFORM=offscreen` + Qt Test 约定）：

1. 组合成员用法：`QWidget` 子类持有 `SControlBridge` 成员，`setTextTr` + 语言切换（`LanguageManager::setLanguage`）后文案按预期重译；
2. `QEvent::LanguageChange` 事件路径：直接 `QCoreApplication::sendEvent` 给目标 widget，验证 `eventFilter` 同样触发重译（对齐 `test_language.cpp` 里 `SControl<Base>` 的等价用例）；
3. `setThemeOverride`/`clearThemeOverride` 对目标 widget 的 `styleSheet()` 生效，语义与 `SControl<Base>` 一致；
4. 操作状态机（触发/超时/复位）在桥接路径下行为与 `test_opstate.cpp` 里 `SControl<Base>` 覆盖的用例等价（复用同一份 `SControlEngine`，做最小冒烟验证即可，不需要重复全部用例）；
5. `slabelAttach(widget)` 返回非空指针，`widget` 销毁后不泄漏（可通过 `QSignalSpy` 监听 `bridge` 的 `destroyed()` 信号验证随 parent 自动析构）；
6. `SControl<Base>` 回归：`tests/test_language.cpp`、`tests/test_scontrol_theme.cpp`、`tests/test_controls.cpp`、`tests/test_opstate.cpp` 现有用例在重构后原样通过，不新增/不修改断言。

`tests/test_font_tokens.cpp`（新增）：

1. 加载 `default.qss`（走 `ThemeManager`），验证 `*[slabelRole="primaryButton"]` 规则文本包含独立的 `font-size` 声明，且其 token 值与 `*` 上的 `@fontSizeBase` 是两个不同的 token 名（防止以后有人手滑把角色规则的 font-size 又写回引用 `@fontSizeBase` 本身，退化回"看起来独立、实际共用"）；
2. 实例级覆盖：`SControl<QPushButton> btn; btn.setFontSizePx(20); QVERIFY(btn.styleSheet().contains("font-size:20px"));`，验证便捷方法生成的 CSS 片段正确。

`tests/CMakeLists.txt` 按现有 `slabel_test(name)` 模式登记以上两个新测试文件。

## 不在本次范围内

- 不为各主题的角色字号发明具体像素值——三个主题里新增的角色字号 token 初始值均沿用原 `@fontSizeBase`，视觉效果与改动前完全一致，具体调整留给主题作者按需修改。
- 不做"运行时给任意未注册 `QWidget*` 查找其 `SControlBridge`"的反向查找 API（如按 widget 指针查桥接对象）——场景二（`slabelAttach`）的调用方自己保存返回的指针即可，YAGNI。
- 不改变 `ThemeManager`/`LanguageManager` 的注册表结构或对外接口。
- 不处理"一个 `QWidget*` 被 `SControlBridge` 重复挂载多次"的去重/防呆——由调用方保证不重复挂载，与现有 `SControl<Base>` 的构造语义（一个控件只构造一次）一致。
