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
