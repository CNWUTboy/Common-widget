# standard_lable —— Qt 基础控件封装库设计文档

- 日期：2026-07-20
- 状态：已评审通过基线，待用户复核
- 命名空间：`slabel`　控件前缀：`S`

---

## 1. 目标与需求

构建一个可复用的 Qt 基础控件封装库，供多个内部应用共用。

### 硬性需求

| 面向 | 要求 |
|---|---|
| Qt 版本 | 同时兼容 Qt 5.15.2 与 Qt 5.15.10 |
| 操作系统 | Windows 7 与 Linux |
| 控件封装 | 封装常用基础控件（按钮、表格、弹窗等），首版核心集约 15 个 |
| 统一风格 + 可切换 | 所有控件风格一致，支持运行时切换主题 |
| 多语言 | 所有控件语言可运行时切换（免重启） |
| 事件绑定 | 控件状态与显示信息支持双向数据绑定，也支持单向状态监听 |

### 设计优先级（用户明确要求）

1. **使用简单** —— 使用方看到的是干净、贴近原生 Qt 的 API。
2. **性能较好** —— 避免运行时遍历树、属性扫描等昂贵机制；尽量编译期绑定、加载期处理。

---

## 2. 关键设计决策

| 面向 | 决定 | 理由 |
|---|---|---|
| 封装方式 | 继承 Qt 控件（`SButton : QPushButton` …） | 迁移成本最低，可直接用于 Qt Designer |
| 核心架构 | CRTP 模板混入 `SControl<Base>` + 三个中央管理器 | 三大能力只写一次，新增控件零重复；编译期绑定，性能好 |
| 风格机制 | 默认 QSS（变量式），代码设定可覆盖 QSS | 分层优先级；设计师改主题不动代码 |
| 多语言 | Qt 原生 `QTranslator` + 运行时即时切换 | 工具链成熟，免重启 |
| 事件绑定 | `Q_PROPERTY` + `NOTIFY` 为骨干，双向 + 单向 | 贴近 Qt 惯例，性能好 |
| 发布形式 | 动态库（`.dll` / `.so`） | 体积小、可独立更新，供多应用共用 |
| 构建工具 | CMake 与 qmake 各提供一份 | 兼容不同团队习惯 |
| C++ 标准 | C++17 | Qt 5.15 支持，模板/绑定实现更清爽 |

---

## 3. 分层架构

```
┌─────────────────────────────────────────────────────┐
│  控件层  SButton / SLineEdit / STableView …           │  ← 使用方直接使用
│         每个 = SControl<QXxx> + Q_OBJECT               │
├─────────────────────────────────────────────────────┤
│  能力层（CRTP 模板 SControl<Base>，共通逻辑写一次）      │
│   · 主题挂钩   · 语言挂钩   · 绑定挂钩                   │
├─────────────────────────────────────────────────────┤
│  服务层（三个单例管理器）                                │
│   ThemeManager · LanguageManager · BindingEngine       │
└─────────────────────────────────────────────────────┘
```

### 3.1 CRTP + moc 的处理方式（核心技术点）

`Q_OBJECT` 宏不能放在模板类中，因此：

- `SControl<Base>` 是**非 moc** 的模板类，承载三大能力的共通逻辑；
- 需要 signal 的场景（如绑定变更通知）由模板内部持有的小型 `SControlCore : QObject` helper 提供 signal；
- 每个**具体控件**（如 `SButton`）才书写 `Q_OBJECT`。

这样 moc 只作用于具体控件，模板负责零重复的共通逻辑。

```cpp
// 能力层：非 moc 模板，写一次
template<class Base>
class SControl : public Base, public ISControl {
public:
    template<typename... Args>
    explicit SControl(Args&&... args) : Base(std::forward<Args>(args)...) {
        ThemeManager::instance().registerControl(this);
        LanguageManager::instance().registerControl(this);
    }
    ~SControl() override {
        ThemeManager::instance().unregisterControl(this);
        LanguageManager::instance().unregisterControl(this);
    }

    // 主题
    void setThemeOverride(const QString& key, const QString& value);
    void clearThemeOverride();

    // 语言：记住源串，切换时自动重译
    void setTextTr(const char* sourceText, const char* context = nullptr);

    // 绑定的属性访问统一走 Qt 元对象（property/setProperty）
protected:
    // Base 的 changeEvent 会转发到这里做重译与重套主题
    void onLanguageChange();
    SControlCore m_core;                 // 提供 signal 的小 QObject
    QHash<QString, QString> m_overrides; // 代码覆盖表
};

// 控件层：具体控件才写 Q_OBJECT
class SLABEL_EXPORT SButton : public SControl<QPushButton> {
    Q_OBJECT
public:
    using SControl<QPushButton>::SControl;
protected:
    void changeEvent(QEvent* e) override; // LanguageChange → onLanguageChange()
};
```

---

## 4. 主题系统

### 4.1 分层优先级

```
最终样式 = QSS 主题文件（底层）  ◄── 代码设定（上层，优先级更高）
```

### 4.2 运作方式

1. **QSS 底层**：`ThemeManager` 加载 `themes/xxx.qss` 应用到 `QApplication`。主题文件支持**变量**（如 `@primary`、`@bg`、`@text`），加载时做字符串替换后再应用。新增主题只需改一组语义色，无需重写整份 QSS。
2. **代码覆盖层**：控件通过 `setThemeOverride(key, value)` 存入自身覆盖表，合成时叠加在 QSS 之上，优先级更高。
3. **切换**：`ThemeManager::setTheme("dark")` → 重新应用 QSS → 已注册控件保留各自的代码覆盖。运行时即时生效，无需重启。

### 4.3 API

```cpp
ThemeManager::instance().setTheme("dark");     // 全局切主题
btn->setThemeOverride("color", "#ff0000");     // 单控件覆盖
btn->clearThemeOverride();                      // 回到 QSS
```

### 4.4 主题文件示例（`themes/dark.qss`）

```css
/* @primary:#3daee9  @bg:#232629  @text:#eff0f1 */
SButton   { background:@primary; color:@text; border-radius:4px; }
SLineEdit { background:@bg; color:@text; border:1px solid @primary; }
```

### 4.5 性能说明

QSS 变量采用「加载期字符串替换」而非每次绘制计算；代码覆盖只在该控件设定时局部重算，避免全树扫描。

### 4.6 自绘控件的主题支持（token API）

QSS 只作用于 Qt 内建可样式化的绘制；**完全自绘的控件**（自定义 `paintEvent` 用 `QPainter` 直接绘制）不会自动跟随 QSS。为让自绘控件也能跟随主题切换，`ThemeManager` 暴露**主题 token 查询 API**：

- 加载主题时，除做字符串替换外，同时把变量头 `/* @k:v ... */` 解析为 token 表。
- `QColor ThemeManager::token(const QString& name) const;` 返回当前主题下该语义色（如 `"primary"`）；未定义返回无效 `QColor`。
- 自绘控件在 `paintEvent` 中查询 token 上色，并连接已有的 `themeChanged(QString)` 信号，在切换时调用 `update()` 重绘。

```cpp
void SSwatch::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), ThemeManager::instance().token("primary"));
}
// 构造中：connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
//                 this, QOverload<>::of(&QWidget::update));
```

---

## 5. 多语言系统

### 5.1 运作方式

- `LanguageManager` 加载 `translations/xxx.qm` → `QApplication::installTranslator` → 向所有顶层 widget 发送 `QEvent::LanguageChange`。
- 每个控件在 `changeEvent` 捕获 `LanguageChange` → 调用 `onLanguageChange()` 重新求值 `tr()` 文案并重套主题。
- **代码动态设定的文案**：`tr()` 只求值一次，故控件通过 `setTextTr(sourceText)` 记住源串，切换时自动重译，解决动态文案不刷新的问题。

### 5.2 API

```cpp
LanguageManager::instance().setLanguage("en");   // 运行时切语言，免重启
btn->setTextTr(QT_TR_NOOP("保存"));               // 记住源串，随语言自动重译
```

### 5.3 语言文件流程

源码中的 `tr()` / `QT_TR_NOOP` → `lupdate` 生成 `.ts` → 翻译 → `lrelease` 生成 `.qm` → 运行时加载。

---

## 6. 事件绑定引擎

### 6.1 机制

以 Qt `Q_PROPERTY` + `NOTIFY` 信号为骨干：每个可绑定属性（如 `value`、`text`、`checked`、`enabled`、`visible`）都有对应的 NOTIFY 信号。`BindingEngine` 在两端之间建立连接，并用**更新中标志**防止双向回写造成无限循环。

数据模型侧提供轻量 `SProperty<T>`（含变更信号）与 `SBindableObject` 基类，供业务对象暴露可绑定字段。

### 6.2 API

```cpp
// 双向：控件属性 ↔ 模型字段，任一端变化自动同步另一端
BindingEngine::bind(edit,  "text",    &user, "name");
BindingEngine::bind(check, "checked", &user, "active");

// 单向：只监听控件状态变化，触发回调，不回写
BindingEngine::observe(slider, "value", [](const QVariant& v){
    qDebug() << "slider =" << v.toInt();
});

// 解绑
BindingEngine::unbind(edit, "text");
```

### 6.3 防循环

双向连接在写入目标端前置 `updating` 标志，写入期间忽略对端回传的 NOTIFY，写完清除。保证 A→B→A 不再触发第二轮。

### 6.4 性能说明

绑定基于 Qt 信号槽的直接连接，无轮询、无属性扫描；连接建立在 `bind()` 时一次完成。

---

## 7. 控件核心集（首版 15 个）

| # | 封装类 | 基类 | 备注 |
|---|---|---|---|
| 1 | SButton | QPushButton | |
| 2 | SLabel | QLabel | |
| 3 | SLineEdit | QLineEdit | |
| 4 | SComboBox | QComboBox | |
| 5 | SCheckBox | QCheckBox | |
| 6 | SRadioButton | QRadioButton | |
| 7 | SSpinBox | QSpinBox | |
| 8 | SSlider | QSlider | |
| 9 | SProgressBar | QProgressBar | |
| 10 | SGroupBox | QGroupBox | |
| 11 | STableView | QTableView | 附一个可绑定的模型辅助 |
| 12 | STreeView | QTreeView | |
| 13 | SListView | QListView | |
| 14 | STabWidget | QTabWidget | |
| 15 | SDialog / SMessageBox | QDialog | 弹窗，含常用消息框封装 |

后续控件按同一 `SControl<QXxx>` 模式扩充，成本极低。

---

## 8. 项目结构

```
standard_lable/
├── CMakeLists.txt          # 主构建（动态库）
├── standard_lable.pro      # qmake 对应
├── include/slabel/         # 对外公开头文件
│   ├── SButton.h  SLineEdit.h  STableView.h …
│   ├── SControl.h          # CRTP 模板
│   ├── SControlCore.h      # 提供 signal 的小 QObject
│   ├── ISControl.h         # 能力接口
│   ├── ThemeManager.h
│   ├── LanguageManager.h
│   ├── BindingEngine.h
│   └── SGlobal.h           # SLABEL_EXPORT 导出宏
├── src/                    # 实现
├── themes/                 # QSS 主题文件（default.qss / dark.qss …）
├── translations/           # .ts / .qm 语言文件
├── examples/               # 展示 App（验证主题/语言/绑定）
└── tests/                  # Qt Test 单元测试
```

---

## 9. 构建与跨平台

### 9.1 动态库导出宏（`SGlobal.h`）

```cpp
#if defined(_WIN32)
#  if defined(SLABEL_BUILD)
#    define SLABEL_EXPORT __declspec(dllexport)
#  else
#    define SLABEL_EXPORT __declspec(dllimport)
#  endif
#else
#  define SLABEL_EXPORT __attribute__((visibility("default")))
#endif
```

### 9.2 双构建系统

- `CMakeLists.txt`：现代主流，Win7/Linux 均良好支持，Qt 5.15 官方支持。
- `standard_lable.pro`：与 Qt Creator 集成最佳，配置简单。
- 两者产物一致（同一份 `src/` 与 `include/`）。

### 9.3 兼容性要点

- **Qt 5.15.2 vs 5.15.10**：同为 5.15 次版本，API 兼容，无需分支处理；CI 同时在两版本编译与测试。
- **Windows 7**：Qt 5.15 官方支持 Win7；使用 MSVC 2019 或 MinGW；发布时随附 Qt 运行时依赖。
- **Linux**：使用 `visibility=default` 控制导出符号。

---

## 10. 测试策略

### 10.1 单元测试（Qt Test）

| 模块 | 用例要点 |
|---|---|
| ThemeManager | 切换主题生效；代码覆盖优先级高于 QSS；`clearThemeOverride` 回退正确 |
| LanguageManager | 运行时切语言，静态与动态文案均刷新；免重启 |
| BindingEngine | 双向同步正确；防循环（A→B→A 不二次触发）；单向监听触发回调；解绑生效 |
| SControl | 注册/注销生命周期；moc 具体控件正常工作 |

### 10.2 集成验证

`examples/` 展示 App：一屏铺满全部核心控件，提供「切主题 / 切语言 / 绑定演示」按钮，手动确认整体一致性与即时切换效果。

---

## 11. 非目标（YAGNI）

- 首版不覆盖 Qt 全部 widget，仅核心 15 个（其余按同模式后续扩充）。
- 不支持 Qt6（需求仅 5.15）。
- 不做静态库形态（仅动态库）。
- 不实现自定义绘制的全新控件，仅封装现有 Qt 控件。

---

## 12. 已知风险与对策

| 风险 | 对策 |
|---|---|
| CRTP + moc 冲突 | `Q_OBJECT` 仅置于具体控件；模板持有 `SControlCore` 提供 signal（见 3.1） |
| 动态文案不随语言刷新 | `setTextTr` 记住源串，`LanguageChange` 时重译（见 5.1） |
| 双向绑定死循环 | `updating` 标志抑制回写（见 6.3） |
| Win7 运行时依赖缺失 | 发布清单明确列出 Qt 运行库；CI 在 Win7 目标验证 |
| 自绘控件不跟随 QSS | 提供主题 token 查询 API + `themeChanged` 重绘（见 4.6、第 13 节） |

---

## 13. 自定义控件支持

库不仅封装 Qt 内建控件，也支持使用方基于 Qt 框架开发的**自定义控件**接入三大能力。

**组合式自定义控件**（继承 `QWidget` 或由标准子控件组合而成）：直接套 CRTP 模板即可获得主题/语言/绑定：

```cpp
class MyChart : public QWidget { /* 使用方已有控件 */ };

class SLABEL_EXPORT SChart : public SControl<MyChart> {
    Q_OBJECT
public:
    using SControl<MyChart>::SControl;
};
```

- 绑定：自定义控件用 `Q_PROPERTY` + `NOTIFY` 暴露属性后，`BindingEngine` 即可用。
- 语言：有 `setText` 者覆写 `changeEvent` 转发 `retranslate()`；否则跳过。
- 主题：组合式控件 QSS 正常生效。

**自绘式自定义控件**（自定义 `paintEvent`）：通过 4.6 的 token API 查询主题色并在 `themeChanged` 时重绘，即可完整跟随主题切换。库在 `examples/` 中提供一个自绘示例控件 `SSwatch` 作为参考实现与验证。
