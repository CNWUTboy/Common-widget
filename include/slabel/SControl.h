#pragma once
#include <QWidget>
#include <QHash>
#include <QString>
#include <utility>
#include "slabel/ISControl.h"
#include "slabel/SControlCore.h"
#include "slabel/ThemeManager.h"

// CRTP 能力模板：把主题/语言/绑定挂钩写一次，套到任意 Qt 基类上。
// 注意：模板类不含 Q_OBJECT；signal 由成员 m_core 提供。
template<class Base>
class SControl : public Base, public ISControl {
public:
    template<typename... Args>
    explicit SControl(Args&&... args) : Base(std::forward<Args>(args)...) {
        ThemeManager::instance().registerControl(this);
    }
    ~SControl() override {
        ThemeManager::instance().unregisterControl(this);
    }

    QWidget* asWidget() override { return this; }
    void retranslate() override {} // Task 5 增强

    // 主题：代码覆盖（widget 级 QSS，优先级高于应用级）
    void setThemeOverride(const QString& key, const QString& value) {
        m_overrides.insert(key, value);
        applyOverrides();
    }
    void clearThemeOverride() {
        m_overrides.clear();
        this->setStyleSheet(QString());
    }

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

    SControlCore m_core;
    QHash<QString, QString> m_overrides;
};
