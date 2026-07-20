#pragma once
#include <QWidget>
#include <QHash>
#include <QString>
#include <type_traits>
#include <utility>
#include "slabel/ISControl.h"
#include "slabel/SControlCore.h"
#include "slabel/ThemeManager.h"

// CRTP 能力模板：把主题/语言/绑定挂钩写一次，套到任意 Qt 基类上。
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
        applyOverrides();
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
