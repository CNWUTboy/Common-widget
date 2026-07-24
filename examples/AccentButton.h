#pragma once
#include <QPushButton>
#include <QEvent>
#include <QByteArray>

// 独立自定义按钮：继承 QPushButton，不依赖 SControl/SControlBridge。
//
// 风格（全局，按类型控制）：本类不重写 paintEvent，外观完全交给全局 QSS。
//   themes/*.qss 中以类名 "AccentButton" 为选择器的规则会命中本类型的所有
//   实例，且比通用的 QPushButton 规则更具体（优先生效）。只改 qss 即可全局
//   统一控制这一类型控件的风格，无需逐个实例写样式代码。
//
// 文字（各自文案，统一随语言切换）：每个实例自设源文案，用原生 tr() 标记，
//   覆写 changeEvent 在 QEvent::LanguageChange 时重译。切语言由 Qt 原生
//   installTranslator 自动派发事件驱动，无需注册到任何管理器。
class AccentButton : public QPushButton {
    Q_OBJECT
public:
    explicit AccentButton(const char* sourceText = "", QWidget* parent = nullptr)
        : QPushButton(parent), m_source(sourceText) {
        retranslate();
    }

    // 设置需随语言切换重译的源文案（登记源串并立即按当前语言显示）
    void setSourceText(const char* sourceText) {
        m_source = sourceText;
        retranslate();
    }

protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange)
            retranslate();
        QPushButton::changeEvent(e);
    }

private:
    void retranslate() {
        if (!m_source.isEmpty())
            setText(tr(m_source.constData()));
    }
    QByteArray m_source; // 保存源文案，供语言切换时按当前语言重新翻译
};
