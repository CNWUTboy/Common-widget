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
        // 未登记处理方式时，视为该控件未接入操作能力，点击保持原生行为
        // （不进入 Busy、不启动超时定时器），避免普通按钮用法被状态机干扰。
        if (!m_opHandler) return;
        setOpState(OperationState::Busy);
        if (m_opTimeoutMs > 0) m_opBusyTimer.start(m_opTimeoutMs);
        m_opHandler();
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
