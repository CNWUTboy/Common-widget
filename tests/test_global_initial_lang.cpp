#include <QtTest>
#include <QApplication>
#include <QPushButton>
#include "slabel/GlobalUiManager.h"

// 派生控件，context = "MyButton"（在 zz.qm 中）
class MyButton : public QPushButton {
    Q_OBJECT
public:
    explicit MyButton(QWidget* p = nullptr) : QPushButton(p) { setText(tr("Go")); }
};

// 验证：install 时指定 initialLanguage != sourceLanguage，
// 事件循环启动后（首帧前）自动整树重译到目标语言。
class TestGlobalInitialLang : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        GlobalUiOptions o;
        o.sourceLanguage = "src";
        o.languageDir = GLOBAL_QM_DIR;
        o.translationSourceDir = GLOBAL_TS_DIR;
        o.initialLanguage = "zz";               // 启动即非源语言
        QVERIFY(GlobalUiManager::install(*qApp, o));
    }

    void startsInInitialLanguage() {
        MyButton mb;
        mb.show();
        QApplication::processEvents();          // 触发 Polish 捕获（此刻源语言=Go）
        QTest::qWait(50);                       // 让 install 排的 singleShot(0) 触发切到 zz
        QCOMPARE(mb.text(), QString("GoZZ"));
        QCOMPARE(GlobalUiManager::instance().currentLanguage(), QString("zz"));
    }
};

QTEST_MAIN(TestGlobalInitialLang)
#include "test_global_initial_lang.moc"
