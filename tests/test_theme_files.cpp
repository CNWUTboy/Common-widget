#include <QtTest>
#include <QFile>
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
    void defaultThemesContainOperationStateSelectors() {
        QFile def(QStringLiteral(THEME_DIR "/default.qss"));
        QVERIFY(def.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString defText = QString::fromUtf8(def.readAll());
        QVERIFY(defText.contains(QStringLiteral("SButton[slabelOperationState=\"busy\"]")));
        QVERIFY(defText.contains(QStringLiteral("SButton[slabelOperationState=\"success\"]")));
        QVERIFY(defText.contains(QStringLiteral("SButton[slabelOperationState=\"failure\"]")));

        QFile dark(QStringLiteral(THEME_DIR "/dark.qss"));
        QVERIFY(dark.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString darkText = QString::fromUtf8(dark.readAll());
        QVERIFY(darkText.contains(QStringLiteral("SButton[slabelOperationState=\"busy\"]")));
        QVERIFY(darkText.contains(QStringLiteral("SButton[slabelOperationState=\"success\"]")));
        QVERIFY(darkText.contains(QStringLiteral("SButton[slabelOperationState=\"failure\"]")));
    }
};

QTEST_MAIN(TestThemeFiles)
#include "test_theme_files.moc"
