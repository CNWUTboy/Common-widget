#include "Gallery.h"
#include "SSwatch.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "slabel/SButton.h"
#include "slabel/SComboBox.h"
#include "slabel/SCheckBox.h"
#include "slabel/SProgressBar.h"
#include "slabel/SGroupBox.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"
#include "slabel/BindingEngine.h"

Gallery::Gallery(QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);

    // 控件展示区
    auto* box = new SGroupBox;
    box->setTitle("控件");
    auto* boxLay = new QVBoxLayout(box);
    boxLay->addWidget(new SButton);
    boxLay->addWidget(new SComboBox);
    boxLay->addWidget(new SCheckBox);
    auto* pb = new SProgressBar; pb->setValue(60);
    boxLay->addWidget(pb);
    boxLay->addWidget(new SSwatch);   // 自绘控件：跟随主题 token 上色
    root->addWidget(box);

    // 绑定演示：输入框 ↔ 镜像标签
    m_edit = new SLineEdit;
    m_mirror = new SLabel;
    BindingEngine::observe(m_edit, "text", [this](const QVariant& v){
        m_mirror->setText(v.toString());
    });
    root->addWidget(m_edit);
    root->addWidget(m_mirror);

    // 操作按钮
    auto* saveBtn = new SButton;
    saveBtn->setTextTr("保存");
    auto* themeBtn = new SButton; themeBtn->setText("切换主题");
    auto* langBtn = new SButton; langBtn->setText("切换语言");
    auto* row = new QHBoxLayout;
    row->addWidget(saveBtn);
    row->addWidget(themeBtn);
    row->addWidget(langBtn);
    root->addLayout(row);

    connect(themeBtn, &QPushButton::clicked, this, [this]{
        m_dark = !m_dark;
        ThemeManager::instance().setTheme(m_dark ? "dark" : "default");
    });
    connect(langBtn, &QPushButton::clicked, this, [this]{
        m_en = !m_en;
        LanguageManager::instance().setLanguage(m_en ? "en" : "zh_CN");
    });
}
