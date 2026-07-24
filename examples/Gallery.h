#pragma once
#include <QWidget>
#include "widgets/SLineEdit.h"
#include "widgets/SLabel.h"

class QGroupBox;
class QPushButton;
class QCheckBox;
class QRadioButton;
class QComboBox;
class QEvent;

class Gallery : public QWidget {
    Q_OBJECT
public:
    explicit Gallery(QWidget* parent = nullptr);
protected:
    // 纯 Qt 原生翻译入口：切换语言时 Qt 自动派发 QEvent::LanguageChange
    void changeEvent(QEvent* e) override;
private:
    // 用 tr() 重设原生控件文案（无任何封装），初始化与语言切换时调用
    void retranslateNative();

    bool m_dark = false;
    bool m_en = false;
    SLineEdit* m_edit = nullptr;
    SLabel* m_mirror = nullptr;

    // 需跟随语言刷新的原生控件：保存指针，在 changeEvent 里重译
    QGroupBox* m_nativeBox = nullptr;
    QPushButton* m_nativeBtn = nullptr;
    QComboBox* m_nativeCombo = nullptr;
    QCheckBox* m_nativeCheck = nullptr;
    QRadioButton* m_nativeRadio = nullptr;
    QGroupBox* m_customBox = nullptr;
};
