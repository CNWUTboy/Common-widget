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
    void constructsWithParent() {
        QWidget parent;
        auto* btn = new SButton(&parent);      // 转发构造合法路径仍工作
        QVERIFY(btn->asWidget() == btn);
        QCOMPARE(btn->parentWidget(), &parent);
    }
};

QTEST_MAIN(TestSControlTheme)
#include "test_scontrol_theme.moc"
