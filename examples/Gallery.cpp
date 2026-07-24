#include "Gallery.h"
#include "SSwatch.h"
#include "StatusLight.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QProgressBar>
#include "widgets/SButton.h"
#include "widgets/SComboBox.h"
#include "widgets/SCheckBox.h"
#include "widgets/SProgressBar.h"
#include "widgets/SGroupBox.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"

Gallery::Gallery(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    // 控件展示区
    auto* box = new SGroupBox;
    box->setProperty("slabelRole", "groupBox");
    box->setTextTr("控件");
    auto* boxLay = new QVBoxLayout(box);
    auto* demoBtn = new SButton; demoBtn->setProperty("slabelRole", "primaryButton");
    boxLay->addWidget(demoBtn);
    auto* combo = new SComboBox; combo->setProperty("slabelRole", "comboBox");
    boxLay->addWidget(combo);
    auto* checkBox = new SCheckBox; checkBox->setProperty("slabelRole", "checkBox");
    boxLay->addWidget(checkBox);
    auto* pb = new SProgressBar; pb->setProperty("slabelRole", "progressBar"); pb->setValue(60);
    boxLay->addWidget(pb);
    boxLay->addWidget(new SSwatch);   // 自绘控件：跟随主题 token 上色
    root->addWidget(box);

    // 用例：纯原生 Qt 控件，不调用任何 setProperty/slabelAttach——
    // 样式：主题 QSS 里的 QPushButton/QLineEdit/... class 选择器对未经任何
    //       接入的原生控件自动生效（零样式代码，含容器 QGroupBox 自身）。
    // 翻译：文案用原生 tr() 标记，切语言由 Gallery::changeEvent 统一重译，
    //       不使用 slabel 的 setTextTr/注册机制。
    m_nativeBox = new QGroupBox;
    auto* nativeLay = new QVBoxLayout(m_nativeBox);

    m_nativeBtn = new QPushButton;
    nativeLay->addWidget(m_nativeBtn);
    nativeLay->addWidget(new QLineEdit);

    m_nativeCombo = new QComboBox;
    m_nativeCombo->addItems({QString(), QString()}); // 文案由 retranslateNative 填充
    nativeLay->addWidget(m_nativeCombo);

    m_nativeCheck = new QCheckBox;
    nativeLay->addWidget(m_nativeCheck);
    m_nativeRadio = new QRadioButton;
    nativeLay->addWidget(m_nativeRadio);

    auto* nativeProgress = new QProgressBar;
    nativeProgress->setValue(60);
    nativeLay->addWidget(nativeProgress);

    root->addWidget(m_nativeBox);

    // 用例：完全独立的自定义控件（不继承 SControl/SControlBridge），
    // 主题靠订阅 themeChanged + colorToken，翻译靠自身 changeEvent + tr()，
    // 均为纯 Qt 原生方式，不需要注册、不需要 setProperty。
    m_customBox = new QGroupBox;
    auto* customLay = new QHBoxLayout(m_customBox);
    auto* lightOnline = new StatusLight;  lightOnline->setStatus(StatusLight::Status::Online);
    auto* lightWarning = new StatusLight; lightWarning->setStatus(StatusLight::Status::Warning);
    auto* lightOffline = new StatusLight; lightOffline->setStatus(StatusLight::Status::Offline);
    customLay->addWidget(lightOnline);
    customLay->addWidget(lightWarning);
    customLay->addWidget(lightOffline);
    customLay->addStretch();
    root->addWidget(m_customBox);

    // 演示：输入框 → 镜像标签，纯原生 Qt 信号槽（不依赖 slabel 绑定能力）
    m_edit = new SLineEdit;
    m_edit->setProperty("slabelRole", "textInput");
    m_mirror = new SLabel;
    connect(m_edit, &QLineEdit::textChanged, this, [this](const QString& t){
        m_mirror->setText(t);
    });
    root->addWidget(m_edit);
    root->addWidget(m_mirror);

    // 操作按钮
    auto* saveBtn = new SButton;
    saveBtn->setProperty("slabelRole", "primaryButton");
    saveBtn->setTextTr("保存");
    auto* themeBtn = new SButton; themeBtn->setProperty("slabelRole", "primaryButton"); themeBtn->setTextTr("切换主题");
    auto* langBtn = new SButton; langBtn->setProperty("slabelRole", "primaryButton"); langBtn->setTextTr("切换语言");
    auto* row = new QHBoxLayout;
    row->addWidget(saveBtn);
    row->addWidget(themeBtn);
    row->addWidget(langBtn);
    root->addLayout(row);

    connect(themeBtn, &QPushButton::clicked, this, [this]{
        m_dark = !m_dark;
        ThemeManager::instance().setTheme(m_dark ? "dark" : "light");
    });
    connect(langBtn, &QPushButton::clicked, this, [this]{
        m_en = !m_en;
        LanguageManager::instance().setLanguage(m_en ? "en" : "zh_CN");
    });

    retranslateNative(); // 初始化原生控件文案
}

void Gallery::retranslateNative() {
    // 纯 Qt 原生方式：用 tr() 重设每个原生控件的文案，无任何封装
    m_nativeBox->setTitle(tr("原生 Qt 控件（零样式代码）"));
    m_nativeBtn->setText(tr("原生按钮"));
    m_nativeCombo->setItemText(0, tr("选项一"));
    m_nativeCombo->setItemText(1, tr("选项二"));
    m_nativeCheck->setText(tr("原生复选框"));
    m_nativeRadio->setText(tr("原生单选框"));
    m_customBox->setTitle(tr("独立自定义控件（不依赖 SControl）"));
}

void Gallery::changeEvent(QEvent* e) {
    // LanguageManager 走原生 installTranslator，切语言时 Qt 自动派发此事件
    if (e->type() == QEvent::LanguageChange)
        retranslateNative();
    QWidget::changeEvent(e);
}
