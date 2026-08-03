#include <QApplication>
#include "Gallery.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // 资源路径用 Qt 资源系统（:/），已嵌入可执行文件，安装/移动后仍可用
    ThemeManager::instance().registerTheme("default", QStringLiteral(":/themes/default.qss"));
    ThemeManager::instance().registerTheme("light", QStringLiteral(":/themes/light.qss"));
    ThemeManager::instance().registerTheme("dark", QStringLiteral(":/themes/dark.qss"));
    ThemeManager::instance().setTheme("default");  // 深蓝为默认风格

    LanguageManager::instance().registerLanguage("zh_CN", QStringLiteral(":/translations/slabel_zh_CN.qm"));
    LanguageManager::instance().registerLanguage("en", QStringLiteral(":/translations/slabel_en.qm"));
    LanguageManager::instance().setLanguage("zh_CN");  // 初始默认中文

    Gallery w;
    w.resize(360, 480);
    w.show();
    return app.exec();
}
