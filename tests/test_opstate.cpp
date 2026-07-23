#include <QtTest>
#include <QPushButton>
#include "slabel/SControl.h"

class TestOpState : public QObject {
    Q_OBJECT
private slots:
    void triggerEntersBusyAndCallsHandlerOnce() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        int calls = 0;
        btn.setOperationHandler([&]{ ++calls; });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        QCOMPARE(calls, 1);
    }
    void repeatedTriggerWhileBusyIsNoOp() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        int calls = 0;
        btn.setOperationHandler([&]{ ++calls; });
        btn.click();
        btn.click();
        btn.click();
        QCOMPARE(calls, 1);
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
    }
    void successThenAutoRevertsToIdle() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        btn.setOperationResetDelayMs(30);
        btn.setOperationHandler([&]{ btn.reportOperationResult(true); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Success));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void failureThenAutoRevertsToIdle() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        btn.setOperationResetDelayMs(30);
        btn.setOperationHandler([&]{ btn.reportOperationResult(false); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Failure));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void reportResultIgnoredWhenNotBusy() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        btn.reportOperationResult(true);  // Idle 时回报，应被忽略
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void timeoutWithoutReportBecomesFailure() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        btn.setOperationTimeoutMs(30);
        btn.setOperationHandler([]{}); // 登记一个什么都不做的 handler，但从不回报结果
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        QTRY_COMPARE(int(btn.operationState()), int(OperationState::Failure));
    }
    void triggerWithoutHandlerIsNoOp() {
        // 回归测试：未登记 handler 时，点击不应进入状态机（保持原生按钮行为）
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void resetOperationStateEarlyFromFailure() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        btn.setOperationResetDelayMs(10000); // 足够长，确保下面的复位不是自动回退触发的
        btn.setOperationHandler([&]{ btn.reportOperationResult(false); });
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Failure));
        btn.resetOperationState();
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
    }
    void resetOperationStateIsNoOpWhenBusyOrIdle() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        btn.resetOperationState(); // Idle 时 no-op
        QCOMPARE(int(btn.operationState()), int(OperationState::Idle));
        btn.setOperationHandler([]{});
        btn.click();
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
        btn.resetOperationState(); // Busy 时 no-op
        QCOMPARE(int(btn.operationState()), int(OperationState::Busy));
    }
    void destroyingBusyControlDoesNotCrash() {
        auto* btn = new SControl<QPushButton>;
        connect(btn, &QPushButton::clicked, btn, [btn]{ btn->triggerOperation(); });
        btn->setOperationHandler([]{});
        btn->click();
        QCOMPARE(int(btn->operationState()), int(OperationState::Busy));
        delete btn; // 不应崩溃
    }
    void operationStateChangedSignalFiresOnEachTransition() {
        SControl<QPushButton> btn;
        connect(&btn, &QPushButton::clicked, &btn, [&]{ btn.triggerOperation(); });
        QSignalSpy spy(&btn.core(), &SControlCore::operationStateChanged);
        btn.setOperationHandler([&]{ btn.reportOperationResult(true); });
        btn.click();
        QCOMPARE(spy.count(), 2); // Idle->Busy, Busy->Success
    }
};

QTEST_MAIN(TestOpState)
#include "test_opstate.moc"
