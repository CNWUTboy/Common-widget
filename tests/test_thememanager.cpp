#include <QtTest>
#include "slabel/ThemeManager.h"

class TestThemeManager : public QObject {
    Q_OBJECT
private slots:
    void substitutesVariables() {
        QString qss = "/* @primary:#ff0000 @bg:#000000 */\n"
                      "SButton { color:@primary; background:@bg; }";
        QString out = ThemeManager::substituteVariables(qss);
        QVERIFY(!out.contains("@primary"));
        QVERIFY(out.contains("color:#ff0000"));
        QVERIFY(out.contains("background:#000000"));
    }
    void unknownVariableLeftUntouched() {
        QString qss = "/* @a:#111 */ X { color:@b; }";
        QString out = ThemeManager::substituteVariables(qss);
        QVERIFY(out.contains("@b")); // 未定义变量保持原样
    }
    void parsesTokens() {
        QString qss = "/* @primary:#ff0000 @bg:#000000 */\nX { }";
        auto tokens = ThemeManager::parseVariables(qss);
        QCOMPARE(tokens.value("primary"), QString("#ff0000"));
        QCOMPARE(tokens.value("bg"), QString("#000000"));
    }
};

QTEST_MAIN(TestThemeManager)
#include "test_thememanager.moc"
