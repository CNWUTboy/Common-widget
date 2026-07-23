# 原生 Qt 控件全局默认样式设计

## 背景

现有主题 QSS（`themes/default.qss`/`dark.qss`/`light.qss`）只对手动打了 `slabelRole` 属性的控件生效（`*[slabelRole="..."]` 属性选择器）。`slabelRole` 从不会被库代码自动设置，无论是项目自定义的 S 系控件还是原生 Qt 控件，业务代码都得逐实例手动 `setProperty("slabelRole", ...)` 才能吃到主题样式——这与"全局统一设置"的诉求相悖。

需求明确为：同一 Qt 原生控件类型统一样式（不区分角色）、零业务代码改动、本次不处理文字/翻译能力。

## 架构决策

不引入新类、新运行时机制。`ThemeManager::setTheme()` 现有的"读QSS → token替换 → `qApp->setStyleSheet()` 全局下发"链路本身就会对应用内所有 `QWidget` 生效，只是目前QSS里没有写针对原生 class 名的选择器。本次只在三个主题QSS文件里补充 7 条 type 选择器规则，内容与对应 `slabelRole` 规则的视觉参数一致，复用同一套 `@token`。

Qt QSS 优先级：属性选择器（`*[slabelRole=...]`）比纯 type 选择器（`QPushButton`）更具体，因此已打角色标签的现有 S 系控件外观不受影响，继续按角色规则渲染；未打标签的原生控件退回吃 type 选择器的默认样式。

`QPushButton` 默认规则不含 `qproperty-icon`（保存图标是"主按钮"角色的语义，不是所有按钮的通用视觉），保留在 `*[slabelRole="primaryButton"]` 里，不下放。

## 组件与改动

只改 `themes/default.qss`、`themes/dark.qss`、`themes/light.qss` 三个文件，各自在现有 `*[slabelRole=...]` 规则块前追加：

```qss
QPushButton    { background:@primary; color:#ffffff; border:none; border-radius:4px; padding:6px 12px; font-size:@fontSizeButton; }
QLineEdit      { background:@bg; color:@text; border:1px solid @border; border-radius:4px; padding:4px; font-size:@fontSizeInput; }
QComboBox      { background:@bg; color:@text; border:1px solid @border; border-radius:4px; font-size:@fontSizeCombo; }
QCheckBox      { background:@bg; color:@text; font-size:@fontSizeCheckBox; }
QRadioButton   { background:@bg; color:@text; font-size:@fontSizeRadioButton; }
QGroupBox      { background:@bg; color:@text; border:1px solid @border; border-radius:4px; margin-top:6px; font-size:@fontSizeGroupBox; }
QProgressBar   { color:@text; border:1px solid @border; border-radius:4px; text-align:center; font-size:@fontSizeProgressBar; }
QProgressBar::chunk { background:@primary; }
```

三个主题文件按各自现有 token 值同构处理，不新增 token 名（沿用角色规则已有的 `@fontSizeButton` 等 7 个字号 token）。

无 C++ 代码改动，无 CMake/pro 文件改动。

## 测试计划

`tests/test_theme_files.cpp` 新增一个测试方法 `defaultThemesContainNativeClassSelectors()`，沿用文件现有"读QSS文本做 `contains` 字符串校验"的模式，对 `default.qss` 与 `dark.qss` 各断言包含以上 7 条 type 选择器（`QPushButton {`、`QLineEdit {` 等）及 `QProgressBar::chunk {`。

## 不在本次范围内

- 不处理原生控件的文字/翻译能力（需要 `SControlBridge`/`slabelAttach`，属于需要业务代码调用的场景，与本次"零改动"目标冲突，另行讨论）。
- 不处理操作状态（busy/success/failure）在原生控件上的自动生效——同样需要显式 attach。
- 不处理第三方/系统对话框内部原生控件（如 `QMessageBox` 的 `QPushButton`）被一并套上全局样式这一"blast radius"——这是全局 class 选择器的必然结果，非本设计引入的新问题，用户已知悉。
