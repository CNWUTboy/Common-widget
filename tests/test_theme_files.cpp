#include <QtTest>
#include <QApplication>
#include "slabel/ThemeManager.h"

class TestThemeFiles : public QObject {
    Q_OBJECT
private slots:
    void loadsDefaultAndDark() {
        ThemeManager::instance().registerTheme("default", QStringLiteral(THEME_DIR "/default.qss"));
        ThemeManager::instance().registerTheme("dark", QStringLiteral(THEME_DIR "/dark.qss"));
        QVERIFY(ThemeManager::instance().setTheme("default"));
        QCOMPARE(ThemeManager::instance().currentTheme(), QString("default"));
        QVERIFY(ThemeManager::instance().setTheme("dark"));
        QCOMPARE(ThemeManager::instance().currentTheme(), QString("dark"));
    }
    void unknownThemeFails() {
        QVERIFY(!ThemeManager::instance().setTheme("nope"));
    }
};

QTEST_MAIN(TestThemeFiles)
#include "test_theme_files.moc"
