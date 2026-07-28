#pragma once
/**
 * @file SControlBridge.h
 * @brief 运行时桥接 SControlBridge：为不便继承 SControl<Base> 的已有控件补齐主题/语言/操作状态能力。
 *
 * 以组合方式包裹目标 QWidget*，并提供 slabelAttach() 自由函数实现零侵入挂接。
 */
#include <QObject>
#include <QWidget>
#include <QString>
#include <QByteArray>
#include <QEvent>
#include <functional>
#include "slabel/SGlobal.h"
#include "slabel/ISControl.h"
#include "slabel/SControlCore.h"
#include "slabel/SControlEngine.h"
#include "slabel/OperationState.h"

/**
 * @brief 面向“已写好、不便改成继承 SControl<Base>”的自定义控件的运行时桥接。
 *
 * 组合一个 SControlBridge 成员（构造时传入目标 QWidget*）即可获得主题/语言/
 * 操作状态能力，或用下面的 slabelAttach() 自由函数零侵入地挂到任意已有控件上。
 */
class SLABEL_EXPORT SControlBridge : public QObject, public ISControl {
    Q_OBJECT
public:
    /**
     * @brief 构造桥接对象，向 ThemeManager/LanguageManager 注册并监听目标事件。
     * @param target 目标控件；非拥有，其生命周期由外部管理。
     * @param parent 父对象，交由 Qt 父子链管理生命周期。
     */
    explicit SControlBridge(QWidget* target, QObject* parent = nullptr);
    /**
     * @brief 析构：从 ThemeManager/LanguageManager 注销。
     */
    ~SControlBridge() override;

    /**
     * @brief 返回被桥接的目标控件（ISControl 接口）。
     * @return 目标 QWidget 指针。
     */
    QWidget* asWidget() override { return m_target; }

    /**
     * @brief 设置需随语言切换重译的源文本，并立即重译一次。
     * @param sourceText 源语言文本（翻译上下文为 "slabel"）。
     *
     * 与 SControl<Base> 语义一致，但走运行时属性查找（不依赖编译期
     * HasSetText），因为目标类型在这里是不透明的 QWidget*。
     */
    void setTextTr(const char* sourceText);
    /**
     * @brief 依据当前语言重译目标控件的 "text" 属性（ISControl 接口）。
     */
    void retranslate() override;

    /**
     * @brief 设置一条主题覆盖项，转发至内部引擎。
     * @param key QSS 属性名。
     * @param value QSS 属性值。
     */
    void setThemeOverride(const QString& key, const QString& value) { m_engine.setThemeOverride(key, value); }
    /**
     * @brief 清空全部主题覆盖项，转发至内部引擎。
     */
    void clearThemeOverride() { m_engine.clearThemeOverride(); }
    /**
     * @brief 设置字号，转发至内部引擎。
     * @param px 字号像素值。
     */
    void setFontSizePx(int px) { m_engine.setFontSizePx(px); }

    /**
     * @brief 访问内部信号载体，用于连接 operationStateChanged 等信号。
     * @return 内部引擎的 SControlCore 引用。
     */
    SControlCore& core() { return m_engine.core(); }

    /**
     * @brief 登记操作处理器，转发至内部引擎。
     * @param handler 触发操作时执行的回调。
     */
    void setOperationHandler(std::function<void()> handler) { m_engine.setOperationHandler(std::move(handler)); }
    /**
     * @brief 回传异步操作结果，转发至内部引擎。
     * @param success true 为成功，false 为失败。
     */
    void reportOperationResult(bool success) { m_engine.reportOperationResult(success); }
    /**
     * @brief 查询当前操作状态。
     * @return 当前 OperationState。
     */
    OperationState operationState() const { return m_engine.operationState(); }
    /**
     * @brief 复位操作状态，转发至内部引擎。
     */
    void resetOperationState() { m_engine.resetOperationState(); }
    /**
     * @brief 设置操作执行超时时长，转发至内部引擎。
     * @param ms 毫秒。
     */
    void setOperationTimeoutMs(int ms) { m_engine.setOperationTimeoutMs(ms); }
    /**
     * @brief 设置成功/失败后自动复位延迟，转发至内部引擎。
     * @param ms 毫秒。
     */
    void setOperationResetDelayMs(int ms) { m_engine.setOperationResetDelayMs(ms); }
    /**
     * @brief 触发一次操作，转发至内部引擎。
     */
    void triggerOperation() { m_engine.triggerOperation(); }

protected:
    /**
     * @brief 事件过滤器：观察目标的 QEvent::LanguageChange 并触发重译。
     * @param obj 事件来源对象。
     * @param e 事件。
     * @return 是否拦截该事件（此处沿用基类默认处理）。
     *
     * SControlBridge 不是目标 widget 本身，不能重写 changeEvent；改用事件
     * 过滤器观察目标的 QEvent::LanguageChange。
     */
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    QWidget* m_target;  // 非拥有
    SControlEngine m_engine;
    QByteArray m_sourceText;
};

/**
 * @brief 零侵入地为已有控件挂接 SControlBridge。
 * @param widget 目标控件，同时作为桥接对象的 parent。
 * @return 新建的 SControlBridge；堆分配，随 widget 的 Qt 父子链自动析构。
 */
SLABEL_EXPORT SControlBridge* slabelAttach(QWidget* widget);
