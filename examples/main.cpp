#include <QApplication>
#include "Gallery.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    ThemeManager::instance().registerTheme("default", QStringLiteral(THEME_DIR "/default.qss"));
    ThemeManager::instance().registerTheme("dark", QStringLiteral(THEME_DIR "/dark.qss"));
    ThemeManager::instance().setTheme("default");

    LanguageManager::instance().registerLanguage("zh_CN", QStringLiteral(TRANS_DIR "/slabel_zh_CN.qm"));
    LanguageManager::instance().registerLanguage("en", QStringLiteral(TRANS_DIR "/slabel_en.qm"));

    Gallery w;
    w.resize(360, 480);
    w.show();
    return app.exec();
}
