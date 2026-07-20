#include <QtTest>
#include <QApplication>
#include "slabel/SButton.h"

class TestSControlTheme : public QObject {
    Q_OBJECT
private slots:
    void overrideSetsWidgetStyleSheet() {
        SButton btn;
        btn.setThemeOverride("color", "#ff0000");
        QVERIFY(btn.styleSheet().contains("color:#ff0000"));
    }
    void clearOverrideEmptiesStyleSheet() {
        SButton btn;
        btn.setThemeOverride("color", "#ff0000");
        btn.clearThemeOverride();
        QVERIFY(btn.styleSheet().isEmpty());
    }
    void multipleOverridesComposed() {
        SButton btn;
        btn.setThemeOverride("color", "#ff0000");
        btn.setThemeOverride("background", "#000000");
        QVERIFY(btn.styleSheet().contains("color:#ff0000"));
        QVERIFY(btn.styleSheet().contains("background:#000000"));
    }
};

QTEST_MAIN(TestSControlTheme)
#include "test_scontrol_theme.moc"
