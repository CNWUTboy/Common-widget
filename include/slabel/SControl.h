#pragma once
#include <QWidget>
#include <QByteArray>
#include <QCoreApplication>
#include <QEvent>
#include <functional>
#include <type_traits>
#include <utility>
#include "slabel/ISControl.h"
#include "slabel/SControlCore.h"
#include "slabel/SControlEngine.h"
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
// 注意：模板类不含 Q_OBJECT；signal 由 m_engine.core() 提供。
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
    explicit SControl(Args&&... args) : Base(std::forward<Args>(args)...), m_engine(this) {
        ThemeManager::instance().registerControl(this);
        LanguageManager::instance().registerControl(this);
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

    // 主题：代码覆盖（widget 级 QSS，优先级高于应用级）——转发到引擎
    void setThemeOverride(const QString& key, const QString& value) { m_engine.setThemeOverride(key, value); }
    void clearThemeOverride() { m_engine.clearThemeOverride(); }
    void setFontSizePx(int px) { m_engine.setFontSizePx(px); }

    // 操作状态反馈能力（RX_OP_STATE）——转发到引擎
    SControlCore& core() { return m_engine.core(); }

    void setOperationHandler(std::function<void()> handler) { m_engine.setOperationHandler(std::move(handler)); }
    void reportOperationResult(bool success) { m_engine.reportOperationResult(success); }
    OperationState operationState() const { return m_engine.operationState(); }
    void resetOperationState() { m_engine.resetOperationState(); }
    void setOperationTimeoutMs(int ms) { m_engine.setOperationTimeoutMs(ms); }
    void setOperationResetDelayMs(int ms) { m_engine.setOperationResetDelayMs(ms); }

    // 具体控件（或自定义控件，含业务层直接实例化的 SControl<T>）把自己的
    // "触发"信号接到这里即可接入操作状态反馈能力，无需继承。
    void triggerOperation() { m_engine.triggerOperation(); }

protected:
    // 语言切换自动重译：任意 Base 通用，业务层直接用 SControl<T> 无需子类
    // 重写 changeEvent 即可获得该能力。
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        Base::changeEvent(e);
    }

    SControlEngine m_engine;
    QByteArray m_sourceText;
};
