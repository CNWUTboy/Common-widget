#include <QtTest>
#include <QFile>
#include <QApplication>
#include <QRegularExpression>
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
        QVERIFY(defText.contains(QStringLiteral("*[slabelOperationState=\"busy\"]")));
        QVERIFY(defText.contains(QStringLiteral("*[slabelOperationState=\"success\"]")));
        QVERIFY(defText.contains(QStringLiteral("*[slabelOperationState=\"failure\"]")));

        QFile dark(QStringLiteral(THEME_DIR "/dark.qss"));
        QVERIFY(dark.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString darkText = QString::fromUtf8(dark.readAll());
        QVERIFY(darkText.contains(QStringLiteral("*[slabelOperationState=\"busy\"]")));
        QVERIFY(darkText.contains(QStringLiteral("*[slabelOperationState=\"success\"]")));
        QVERIFY(darkText.contains(QStringLiteral("*[slabelOperationState=\"failure\"]")));
    }
    void defaultThemesContainNativeClassSelectors() {
        const QStringList classNames = {
            QStringLiteral("QPushButton"),  QStringLiteral("QLineEdit"),
            QStringLiteral("QComboBox"),    QStringLiteral("QCheckBox"),
            QStringLiteral("QRadioButton"), QStringLiteral("QGroupBox"),
            QStringLiteral("QProgressBar"), QStringLiteral("QProgressBar::chunk"),
        };

        QFile def(QStringLiteral(THEME_DIR "/default.qss"));
        QVERIFY(def.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString defText = QString::fromUtf8(def.readAll());
        for (const auto& cls : classNames)
            QVERIFY(QRegularExpression(QRegularExpression::escape(cls) + QStringLiteral("\\s*\\{"))
                        .match(defText).hasMatch());

        QFile dark(QStringLiteral(THEME_DIR "/dark.qss"));
        QVERIFY(dark.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString darkText = QString::fromUtf8(dark.readAll());
        for (const auto& cls : classNames)
            QVERIFY(QRegularExpression(QRegularExpression::escape(cls) + QStringLiteral("\\s*\\{"))
                        .match(darkText).hasMatch());
    }
    void defaultThemesContainFontAndIconTokens() {
        QFile def(QStringLiteral(THEME_DIR "/default.qss"));
        QVERIFY(def.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString defText = QString::fromUtf8(def.readAll());
        QVERIFY(defText.contains(QStringLiteral("font-family")));
        QVERIFY(defText.contains(QStringLiteral(":/icons/light/save.svg")));

        QFile dark(QStringLiteral(THEME_DIR "/dark.qss"));
        QVERIFY(dark.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString darkText = QString::fromUtf8(dark.readAll());
        QVERIFY(darkText.contains(QStringLiteral("font-family")));
        QVERIFY(darkText.contains(QStringLiteral(":/icons/dark/save.svg")));
    }
};

QTEST_MAIN(TestThemeFiles)
#include "test_theme_files.moc"
