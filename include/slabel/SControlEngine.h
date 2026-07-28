#pragma once
/**
 * @file SControlEngine.h
 * @brief 控件通用机制引擎 SControlEngine：与具体控件类型无关的主题覆盖表 + 操作状态机。
 *
 * 从 SControl<Base> 抽取，供 SControl<Base> 与 SControlBridge 复用；不负责文本重译，
 * 也不实现 ISControl、不向 ThemeManager/LanguageManager 注册。
 */

#include <QWidget>
#include <QHash>
#include <QString>
#include <QTimer>
#include <functional>
#include "slabel/SGlobal.h"
#include "slabel/SControlCore.h"
#include "slabel/OperationState.h"

/**
 * @brief 从 SControl<Base> 中抽取的、与具体控件类型无关的机制：主题覆盖表 + 操作状态机。
 *
 * 不含文本重译（HasSetText 编译期分支留在 SControl<Base> 内，
 * SControlBridge 走运行时属性查找的独立实现），也不实现 ISControl、不向
 * ThemeManager/LanguageManager 注册——这些契约由持有者（SControl<Base> 或
 * SControlBridge）负责，避免重复 retranslate() 调用、破坏 asWidget() 语义。
 */
class SLABEL_EXPORT SControlEngine {
public:
    /**
     * @brief 构造引擎并初始化操作状态机的定时器。
     * @param widget 目标控件；非拥有，其生命周期由持有者管理。
     */
    explicit SControlEngine(QWidget* widget);
    /**
     * @brief 析构引擎。
     */
    ~SControlEngine();

    /**
     * @brief 设置一条主题覆盖项（widget 级 QSS，优先级高于应用级）。
     * @param key QSS 属性名，如 "font-size"、"color"。
     * @param value 对应的 QSS 属性值。
     */
    void setThemeOverride(const QString& key, const QString& value);
    /**
     * @brief 清空全部主题覆盖项，恢复为应用级主题。
     */
    void clearThemeOverride();
    /**
     * @brief 单独设置字号的便捷入口。
     * @param px 字号像素值；等价于 setThemeOverride("font-size", "<px>px")。
     */
    void setFontSizePx(int px);

    /**
     * @brief 访问内部信号载体，用于连接 operationStateChanged 等信号。
     * @return 内部 SControlCore 引用。
     */
    SControlCore& core() { return m_core; }  // 暴露 operationStateChanged 等信号

    /**
     * @brief 登记操作处理器；未登记时 triggerOperation 保持控件原生行为。
     * @param handler 触发操作时执行的回调。
     */
    void setOperationHandler(std::function<void()> handler);
    /**
     * @brief 回传异步操作结果，仅在“执行中(Busy)”时生效，迟到/多余的回报会被忽略。
     * @param success true 置为成功状态，false 置为失败状态。
     */
    void reportOperationResult(bool success);
    /**
     * @brief 查询当前操作状态。
     * @return 当前 OperationState。
     */
    OperationState operationState() const { return m_opState; }
    /**
     * @brief 复位操作状态：仅在“成功”/“失败”时生效，提前恢复为“待命(Idle)”。
     */
    void resetOperationState();
    /**
     * @brief 设置操作执行超时时长。
     * @param ms 毫秒；超时未回报结果则自动判为失败，<=0 表示不启用超时。
     */
    void setOperationTimeoutMs(int ms) { m_opTimeoutMs = ms; }
    /**
     * @brief 设置成功/失败后自动复位为待命的延迟时长。
     * @param ms 毫秒；<=0 表示不自动复位。
     */
    void setOperationResetDelayMs(int ms) { m_opResetDelayMs = ms; }
    /**
     * @brief 触发一次操作：进入执行中并调用已登记的处理器，重复触发不产生新操作。
     */
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
