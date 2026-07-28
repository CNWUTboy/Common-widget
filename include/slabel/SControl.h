#pragma once
/**
 * @file SControl.h
 * @brief CRTP 能力模板 SControl：把主题/语言/绑定/操作状态挂钩一次性套到任意 Qt 基类上。
 *
 * 通过继承目标 Qt 基类并实现 ISControl 接口，为任意控件统一注入主题切换、
 * 语言自动重译与操作状态反馈能力，业务层可直接实例化 SControl<T> 无需再派生子类。
 */
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
/**
 * @brief 编译期探测类型 T 是否具备 setText(QString) 接口。
 * @tparam T 待探测的类型。
 *
 * 无文本控件（如 QComboBox、QSpinBox、QTableView 等）没有此接口，retranslate()
 * 需据此在编译期分支，否则虚函数隐式实例化时会因找不到 setText 而编译失败。
 */
template<class T, class = void>
struct HasSetText : std::false_type {};
template<class T>
struct HasSetText<T, std::void_t<decltype(std::declval<T&>().setText(QString()))>>
    : std::true_type {};
}

/**
 * @brief CRTP 能力模板：把主题/语言/绑定/操作状态挂钩写一次，套到任意 Qt 基类上。
 * @tparam Base 目标 Qt 控件基类（如 QPushButton、QComboBox 等）。
 *
 * 注意：模板类不含 Q_OBJECT；signal 由 m_engine.core() 提供。业务层直接实例化
 * SControl<Base> 即可获得主题覆盖、语言自动重译与操作状态反馈能力，无需继承。
 */

// CRTP 能力模板：把主题/语言/绑定/操作状态挂钩写一次，套到任意 Qt 基类上。
// 注意：模板类不含 Q_OBJECT；signal 由 m_engine.core() 提供。
template<class Base>
class SControl : public Base, public ISControl {
public:
    /**
     * @brief 转发引用构造：将全部参数透传给 Base 构造并初始化引擎。
     * @tparam Args 转发给 Base 构造函数的参数包。
     *
     * 为避免以非 const 左值调用时把拷贝/移动构造"劫持"掉（从而产生费解的深层
     * 模板报错），对单参数且该参数本身是 SControl 派生/同类的情况做 SFINAE
     * 排除，让编译器改选（被删除的）拷贝构造，报错更清晰。构造时自动向
     * ThemeManager 与 LanguageManager 注册本控件。
     */
    template<typename... Args,
             typename = std::enable_if_t<
                 !(sizeof...(Args) == 1 &&
                   (std::is_base_of_v<SControl, std::decay_t<Args>> || ...))>>
    explicit SControl(Args&&... args) : Base(std::forward<Args>(args)...), m_engine(this) {
        ThemeManager::instance().registerControl(this);
        LanguageManager::instance().registerControl(this);
    }
    /**
     * @brief 析构：从 ThemeManager 与 LanguageManager 注销本控件。
     */
    ~SControl() override {
        ThemeManager::instance().unregisterControl(this);
        LanguageManager::instance().unregisterControl(this);
    }

    /**
     * @brief 返回本控件的 QWidget* 视图（ISControl 接口实现）。
     * @return 指向自身的 QWidget 指针。
     */
    QWidget* asWidget() override { return this; }

    /**
     * @brief 设置可翻译文本：记住源串，并立即按当前语言重译。
     * @param sourceText 源语言文本（作为 tr 的源键）。
     */
    void setTextTr(const char* sourceText) {
        m_sourceText = QByteArray(sourceText);
        retranslate();
    }
    /**
     * @brief 按当前语言重译并回填控件文本（ISControl 接口实现）。
     * @note 仅当 Base 具备 setText 接口且源串非空时才生效（编译期分支）。
     */
    void retranslate() override {
        if constexpr (slabel_detail::HasSetText<Base>::value) {
            if (!m_sourceText.isEmpty())
                this->setText(QCoreApplication::translate("slabel", m_sourceText.constData()));
        }
    }

    /**
     * @brief 设置主题覆盖（widget 级 QSS，优先级高于应用级）。转发到引擎。
     * @param key QSS 属性键。
     * @param value QSS 属性值。
     */
    void setThemeOverride(const QString& key, const QString& value) { m_engine.setThemeOverride(key, value); }
    /**
     * @brief 清除本控件的所有主题覆盖，恢复应用级主题。转发到引擎。
     */
    void clearThemeOverride() { m_engine.clearThemeOverride(); }
    /**
     * @brief 设置字体像素大小（widget 级覆盖）。转发到引擎。
     * @param px 字体大小（像素）。
     */
    void setFontSizePx(int px) { m_engine.setFontSizePx(px); }

    /**
     * @brief 获取操作状态核心对象（提供 RX_OP_STATE 相关信号）。转发到引擎。
     * @return 引擎内部的 SControlCore 引用。
     */
    SControlCore& core() { return m_engine.core(); }

    /**
     * @brief 设置操作触发时执行的处理函数。转发到引擎。
     * @param handler 触发操作时调用的回调。
     */
    void setOperationHandler(std::function<void()> handler) { m_engine.setOperationHandler(std::move(handler)); }
    /**
     * @brief 上报异步操作结果，驱动状态从进行中切换到成功/失败。转发到引擎。
     * @param success true 表示成功，false 表示失败。
     */
    void reportOperationResult(bool success) { m_engine.reportOperationResult(success); }
    /**
     * @brief 查询当前操作状态。转发到引擎。
     * @return 当前 OperationState。
     */
    OperationState operationState() const { return m_engine.operationState(); }
    /**
     * @brief 复位操作状态到空闲。转发到引擎。
     */
    void resetOperationState() { m_engine.resetOperationState(); }
    /**
     * @brief 设置操作超时时长（毫秒）。转发到引擎。
     * @param ms 超时毫秒数。
     */
    void setOperationTimeoutMs(int ms) { m_engine.setOperationTimeoutMs(ms); }
    /**
     * @brief 设置操作完成后自动复位的延时（毫秒）。转发到引擎。
     * @param ms 复位延时毫秒数。
     */
    void setOperationResetDelayMs(int ms) { m_engine.setOperationResetDelayMs(ms); }

    /**
     * @brief 触发一次操作（进入进行中状态并调用处理函数）。转发到引擎。
     *
     * 具体控件（或自定义控件，含业务层直接实例化的 SControl<T>）把自己的
     * "触发"信号接到这里即可接入操作状态反馈能力，无需继承。
     */
    void triggerOperation() { m_engine.triggerOperation(); }

protected:
    /**
     * @brief 事件处理：拦截 LanguageChange 事件自动重译。
     * @param e 事件对象。
     *
     * 语言切换自动重译：任意 Base 通用，业务层直接用 SControl<T> 无需子类
     * 重写 changeEvent 即可获得该能力。
     */
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        Base::changeEvent(e);
    }

    SControlEngine m_engine;
    QByteArray m_sourceText;
};
