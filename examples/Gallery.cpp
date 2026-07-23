#include "Gallery.h"
#include "SSwatch.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include "slabel/BindingEngine.h"

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
    // 验证主题 QSS 里新增的 QPushButton/QLineEdit/QComboBox/QCheckBox/
    // QRadioButton/QGroupBox/QProgressBar class 选择器对未经任何接入的
    // 原生控件也自动生效（含容器自身：这里就是一个原生 QGroupBox）。
    auto* nativeBox = new QGroupBox(QStringLiteral("原生 Qt 控件（零样式代码）"));
    auto* nativeLay = new QVBoxLayout(nativeBox);

    nativeLay->addWidget(new QPushButton(QStringLiteral("原生按钮")));
    nativeLay->addWidget(new QLineEdit);

    auto* nativeCombo = new QComboBox;
    nativeCombo->addItems({QStringLiteral("选项一"), QStringLiteral("选项二")});
    nativeLay->addWidget(nativeCombo);

    nativeLay->addWidget(new QCheckBox(QStringLiteral("原生复选框")));
    nativeLay->addWidget(new QRadioButton(QStringLiteral("原生单选框")));

    auto* nativeProgress = new QProgressBar;
    nativeProgress->setValue(60);
    nativeLay->addWidget(nativeProgress);

    root->addWidget(nativeBox);

    // 绑定演示：输入框 ↔ 镜像标签
    m_edit = new SLineEdit;
    m_edit->setProperty("slabelRole", "textInput");
    m_mirror = new SLabel;
    BindingEngine::observe(m_edit, "text", [this](const QVariant& v){
        m_mirror->setText(v.toString());
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
}
