#include <QtTest>
#include <QApplication>
#include <QLabel>
#include "slabel/GlobalUiManager.h"

// 验证：主题经 install 后对未修改的原生控件全局生效（qApp 全局 QSS）。
class TestGlobalTheme : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        GlobalUiOptions o;
        o.themeDir = THEME_DIR;  // 构建目录的 themes（含 default/dark/light.qss）
        QVERIFY(GlobalUiManager::install(*qApp, o));
    }

    void themeAppliesGloballyAndSwitches() {
        QLabel unmodified("x");  // 未经任何接入的原生控件
        Q_UNUSED(unmodified);

        QVERIFY(GlobalUiManager::instance().setTheme("dark"));
        const QString darkSheet = qApp->styleSheet();
        QVERIFY(!darkSheet.isEmpty());
        QCOMPARE(GlobalUiManager::instance().currentTheme(), QString("dark"));

        QVERIFY(GlobalUiManager::instance().setTheme("default"));
        QVERIFY(qApp->styleSheet() != darkSheet); // 切换后全局样式表确实改变
    }

    void installIsOneShot() {
        GlobalUiOptions o;
        QVERIFY(!GlobalUiManager::install(*qApp, o)); // 重复安装返回 false
    }
};

QTEST_MAIN(TestGlobalTheme)
#include "test_global_theme.moc"
