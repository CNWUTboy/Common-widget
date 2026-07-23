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
