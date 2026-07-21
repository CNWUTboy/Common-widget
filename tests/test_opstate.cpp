#include <QtTest>
#include "slabel/SButton.h"

class TestOpState : public QObject {
    Q_OBJECT
private slots:
    void triggerEntersBusyAndCallsHandlerOnce() {
        SButton btn;
        int calls = 0;
        btn.setOperationHandler([&]{ ++calls; });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        QCOMPARE(calls, 1);
    }
    void repeatedTriggerWhileBusyIsNoOp() {
        SButton btn;
        int calls = 0;
        btn.setOperationHandler([&]{ ++calls; });
        btn.click();
        btn.click();
        btn.click();
        QCOMPARE(calls, 1);
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
    }
    void successThenAutoRevertsToIdle() {
        SButton btn;
        btn.setOperationResetDelayMs(30);
        btn.setOperationHandler([&]{ btn.reportOperationResult(true); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Success));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void failureThenAutoRevertsToIdle() {
        SButton btn;
        btn.setOperationResetDelayMs(30);
        btn.setOperationHandler([&]{ btn.reportOperationResult(false); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Failure));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void reportResultIgnoredWhenNotBusy() {
        SButton btn;
        btn.reportOperationResult(true);  // Idle 时回报，应被忽略
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void timeoutWithoutReportBecomesFailure() {
        SButton btn;
        btn.setOperationTimeoutMs(30);
        btn.click(); // 不登记 handler，也不回报结果
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Failure));
    }
    void resetOperationStateEarlyFromFailure() {
        SButton btn;
        btn.setOperationResetDelayMs(10000); // 足够长，确保下面的复位不是自动回退触发的
        btn.setOperationHandler([&]{ btn.reportOperationResult(false); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Failure));
        btn.resetOperationState();
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void resetOperationStateIsNoOpWhenBusyOrIdle() {
        SButton btn;
        btn.resetOperationState(); // Idle 时 no-op
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
        btn.setOperationHandler([]{});
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        btn.resetOperationState(); // Busy 时 no-op
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
    }
    void destroyingBusyControlDoesNotCrash() {
        auto* btn = new SButton;
        btn->setOperationHandler([]{});
        btn->click();
        QCOMPARE(int(btn->operationState()), int(OperationState::Busy));
        delete btn; // 不应崩溃
    }
    void operationStateChangedSignalFiresOnEachTransition() {
        SButton btn;
        QSignalSpy spy(&btn.core(), &SControlCore::operationStateChanged);
        btn.setOperationHandler([&]{ btn.reportOperationResult(true); });
        btn.click();
        QCOMPARE(spy.count(), 2); // Idle->Busy, Busy->Success
    }
};

QTEST_MAIN(TestOpState)
#include "test_opstate.moc"
