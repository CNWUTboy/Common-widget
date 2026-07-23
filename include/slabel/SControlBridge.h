#pragma once
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

// 面向"已经写好、不方便改成继承 SControl<Base>"的自定义控件类的运行时桥接：
// 组合一个 SControlBridge 成员（构造时传入目标 QWidget*）即可获得主题/语言/
// 操作状态能力，或用下面的 slabelAttach() 自由函数零侵入地挂到任意已有控件上。
class SLABEL_EXPORT SControlBridge : public QObject, public ISControl {
    Q_OBJECT
public:
    explicit SControlBridge(QWidget* target, QObject* parent = nullptr);
    ~SControlBridge() override;

    QWidget* asWidget() override { return m_target; }

    // 语言：与 SControl<Base> 语义一致，但走运行时属性查找（不依赖编译期
    // HasSetText），因为目标类型在这里是不透明的 QWidget*。
    void setTextTr(const char* sourceText);
    void retranslate() override;

    void setThemeOverride(const QString& key, const QString& value) { m_engine.setThemeOverride(key, value); }
    void clearThemeOverride() { m_engine.clearThemeOverride(); }
    void setFontSizePx(int px) { m_engine.setFontSizePx(px); }

    SControlCore& core() { return m_engine.core(); }

    void setOperationHandler(std::function<void()> handler) { m_engine.setOperationHandler(std::move(handler)); }
    void reportOperationResult(bool success) { m_engine.reportOperationResult(success); }
    OperationState operationState() const { return m_engine.operationState(); }
    void resetOperationState() { m_engine.resetOperationState(); }
    void setOperationTimeoutMs(int ms) { m_engine.setOperationTimeoutMs(ms); }
    void setOperationResetDelayMs(int ms) { m_engine.setOperationResetDelayMs(ms); }
    void triggerOperation() { m_engine.triggerOperation(); }

protected:
    // SControlBridge 不是目标 widget 本身，不能重写 changeEvent；改用事件
    // 过滤器观察目标的 QEvent::LanguageChange。
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    QWidget* m_target;  // 非拥有
    SControlEngine m_engine;
    QByteArray m_sourceText;
};

// 零侵入挂接：堆分配，parent = widget，随 widget 的 Qt 父子链自动析构。
SLABEL_EXPORT SControlBridge* slabelAttach(QWidget* widget);
