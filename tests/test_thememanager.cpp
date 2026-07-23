#include <QtTest>
#include <QDir>
#include <QTemporaryFile>
#include <QSignalSpy>
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
    void prefixVariablesNotClobbered() {
        QString qss = "/* @primary:#ff0000 @primaryDark:#800000 */\n"
                      "X { a:@primaryDark; b:@primary; }";
        QString out = ThemeManager::substituteVariables(qss);
        QVERIFY(out.contains("a:#800000"));
        QVERIFY(out.contains("b:#ff0000"));
        QVERIFY(!out.contains("@"));
    }
    void parsesRgbaValue() {
        QString qss = "/* @overlay:rgba(0,0,0,0.5) */\nX { }";
        auto tokens = ThemeManager::parseVariables(qss);
        QCOMPARE(tokens.value("overlay"), QString("rgba(0,0,0,0.5)"));
    }
    void parsesQuotedStringValues() {
        // 字体名含空格、图标路径含 : 和 /，都需要引号包裹才能被正确解析
        QString qss = "/* @fontFamily:\"Microsoft YaHei\" @iconSave:\":/icons/dark/save.svg\" */\nX { }";
        auto tokens = ThemeManager::parseVariables(qss);
        QCOMPARE(tokens.value("fontFamily"), QString("Microsoft YaHei")); // 引号被剥离
        QCOMPARE(tokens.value("iconSave"), QString(":/icons/dark/save.svg"));
    }
    void tokenColorTokenIconTokenReflectLoadedTheme() {
        QTemporaryFile f(QDir::tempPath() + "/slabel_test_theme_XXXXXX.qss");
        QVERIFY(f.open());
        f.write("/* @primary:#ff0000 @fontFamily:\"Microsoft YaHei\" "
                "@iconSave:\":/icons/dark/save.svg\" */\nX { }");
        f.close();

        auto& tm = ThemeManager::instance();
        tm.registerTheme("qa_theme", f.fileName());
        QVERIFY(tm.setTheme("qa_theme"));

        QCOMPARE(tm.token("fontFamily"), QString("Microsoft YaHei")); // token() 原样返回，不做类型转换
        QCOMPARE(tm.token("iconSave"), QString(":/icons/dark/save.svg"));
        QCOMPARE(tm.colorToken("primary"), QColor("#ff0000"));
        QVERIFY(tm.iconToken("no_such_key").isNull()); // 未定义 token -> 空路径 -> null QIcon
        QVERIFY(tm.token("no_such_key").isEmpty()); // 未定义 token 返回空字符串
    }
    void setThemeEmitsThemeChangedSignal() {
        QTemporaryFile f(QDir::tempPath() + "/slabel_test_theme_XXXXXX.qss");
        QVERIFY(f.open());
        f.write("/* @primary:#00ff00 */\nX { color:@primary; }");
        f.close();

        auto& tm = ThemeManager::instance();
        tm.registerTheme("qa_theme_signal", f.fileName());
        QSignalSpy spy(&tm, &ThemeManager::themeChanged);
        QVERIFY(tm.setTheme("qa_theme_signal"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), QString("qa_theme_signal"));
    }
};

QTEST_MAIN(TestThemeManager)
#include "test_thememanager.moc"
