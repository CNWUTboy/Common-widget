#include <QtTest>
#include <QPushButton>
#include <QSignalSpy>
#include "slabel/SControlEngine.h"

// SControlEngine 是 SControl<Base> 与 SControlBridge 共用的内部引擎，平时只
// 通过这两个持有者间接被使用。这里直接实例化它，覆盖其完整公开接口。
class TestControlEngine : public QObject {
    Q_OBJECT
private slots:
    void themeOverrideAppliesToWidgetStyleSheet() {
        QPushButton btn;
        SControlEngine engine(&btn);
        engine.setThemeOverride("color", "#ff0000");
        QVERIFY(btn.styleSheet().contains("color:#ff0000"));
        engine.clearThemeOverride();
        QVERIFY(btn.styleSheet().isEmpty());
    }
    void setFontSizePxAppliesInlineOverride() {
        QPushButton btn;
        SControlEngine engine(&btn);
        engine.setFontSizePx(18);
        QVERIFY(btn.styleSheet().contains("font-size:18px"));
    }
    void operationStateMachineSmokeTest() {
        QPushButton btn;
        SControlEngine engine(&btn);
        QSignalSpy spy(&engine.core(), &SControlCore::operationStateChanged);
        int calls = 0;
        engine.setOperationHandler([&]{ ++calls; });
        engine.triggerOperation();
        QCOMPARE(calls, 1);
        QCOMPARE(int(engine.operationState()), int(OperationState::Busy));
        engine.reportOperationResult(true);
        QCOMPARE(int(engine.operationState()), int(OperationState::Success));
        QCOMPARE(spy.count(), 2); // Idle->Busy, Busy->Success
    }
    void resetOperationStateEarlyFromFailure() {
        QPushButton btn;
        SControlEngine engine(&btn);
        engine.setOperationResetDelayMs(10000); // 足够长，确保下面的复位不是自动回退触发的
        engine.setOperationHandler([&]{ engine.reportOperationResult(false); });
        engine.triggerOperation();
        QCOMPARE(int(engine.operationState()), int(OperationState::Failure));
        engine.resetOperationState();
        QCOMPARE(int(engine.operationState()), int(OperationState::Idle));
    }
    void timeoutWithoutReportBecomesFailure() {
        QPushButton btn;
        SControlEngine engine(&btn);
        engine.setOperationTimeoutMs(30);
        engine.setOperationHandler([]{}); // 登记一个什么都不做的 handler，但从不回报结果
        engine.triggerOperation();
        QCOMPARE(int(engine.operationState()), int(OperationState::Busy));
        QTRY_COMPARE(int(engine.operationState()), int(OperationState::Failure));
    }
};

QTEST_MAIN(TestControlEngine)
#include "test_control_engine.moc"
