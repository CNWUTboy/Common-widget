// 零接入全局管理演示：全部使用「未修改的原生控件」——只用 tr() 设文字、
// 不覆写 changeEvent、不继承任何 slabel 基类、不做任何注册。
// main() 里仅 GlobalUiManager::install(...) 一行，之后切主题/切语言全走库，
// 由库自动完成对这些原生控件的样式与文字更新。
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include "slabel/GlobalUiManager.h"

class DemoWindow : public QWidget {
    Q_OBJECT
public:
    DemoWindow() {
        setWindowTitle(tr("零接入全局管理演示"));
        auto* root = new QVBoxLayout(this);

        auto* box = new QGroupBox(tr("原生控件（未做任何接入）"));
        auto* boxLay = new QVBoxLayout(box);
        boxLay->addWidget(new QLabel(tr("这是一个原生标签")));
        auto* edit = new QLineEdit;
        edit->setPlaceholderText(tr("请输入内容"));
        boxLay->addWidget(edit);
        auto* combo = new QComboBox;
        combo->addItem(tr("选项一"));
        combo->addItem(tr("选项二"));
        boxLay->addWidget(combo);
        boxLay->addWidget(new QPushButton(tr("保存")));
        root->addWidget(box);

        // 控制按钮：切主题 / 切语言——均走 GlobalUiManager，全局生效
        auto* themeBtn = new QPushButton(tr("切换主题"));
        auto* langBtn = new QPushButton(tr("切换语言"));
        auto* row = new QHBoxLayout;
        row->addWidget(themeBtn);
        row->addWidget(langBtn);
        root->addLayout(row);

        connect(themeBtn, &QPushButton::clicked, this, [this] {
            m_dark = !m_dark;
            GlobalUiManager::instance().setTheme(m_dark ? "dark" : "default");
        });
        connect(langBtn, &QPushButton::clicked, this, [this] {
            m_en = !m_en;
            GlobalUiManager::instance().setLanguage(m_en ? "en" : "zh_CN");
        });
    }
private:
    bool m_dark = false;
    bool m_en = false;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // 契约：install 最先调用（QApplication 之后、任何 UI 构造之前）
    // 资源路径用 Qt 资源系统（:/），已嵌入可执行文件，安装/移动后仍可用
    GlobalUiOptions opt;
    opt.themeDir = ":/themes";                // 嵌入的 themes（default/light/dark）
    opt.languageDir = ":/translations";       // 嵌入的 en.qm
    opt.translationSourceDir = ":/ts";        // 嵌入的 en.ts（源串反查目录）
    opt.sourceLanguage = "zh_CN";             // 源码文案为中文
    opt.initialTheme = "default";
    GlobalUiManager::install(app, opt);

    DemoWindow w;
    w.resize(360, 240);
    w.show();
    return app.exec();
}

#include "global_demo.moc"
