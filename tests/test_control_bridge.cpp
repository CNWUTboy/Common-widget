#include <QtTest>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QCoreApplication>
#include "slabel/SControlBridge.h"

// 组合式用法的参考实现：业务层自定义控件类，把 SControlBridge 当成员用。
class BridgedLabel : public QWidget {
public:
    explicit BridgedLabel(QWidget* parent = nullptr)
        : QWidget(parent), label(this), bridge(&label) {}
    QLabel label;
    SControlBridge bridge;
};

class TestControlBridge : public QObject {
    Q_OBJECT
private slots:
    void composedMemberSetTextTrAppliesToTarget() {
        BridgedLabel w;
        w.bridge.setTextTr("Save");
        QCOMPARE(w.label.text(), QString("Save"));
        w.bridge.retranslate();
        QCOMPARE(w.label.text(), QString("Save"));
    }
    void changeEventOnTargetTriggersRetranslateViaEventFilter() {
        // 验证 SControlBridge 用 installEventFilter 替代 changeEvent 重写
        // （因为它不是目标 QWidget 本身）依然能捕获语言切换事件。
        QLabel label;
        SControlBridge bridge(&label);
        bridge.setTextTr("Save");
        QCOMPARE(label.text(), QString("Save"));
        label.setText(QStringLiteral("mutated"));
        QEvent ev(QEvent::LanguageChange);
        QCoreApplication::sendEvent(&label, &ev);
        QCOMPARE(label.text(), QString("Save"));
    }
    void themeOverrideAffectsTargetStyleSheet() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        bridge.setThemeOverride("color", "#ff0000");
        QVERIFY(btn.styleSheet().contains("color:#ff0000"));
        bridge.clearThemeOverride();
        QVERIFY(btn.styleSheet().isEmpty());
    }
    void operationStateMachineSmokeTestViaBridge() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        int calls = 0;
        bridge.setOperationHandler([&]{ ++calls; });
        bridge.triggerOperation();
        QCOMPARE(calls, 1);
        QCOMPARE(int(bridge.operationState()), int(OperationState::Busy));
    }
    void asWidgetReturnsTarget() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        QVERIFY(bridge.asWidget() == &btn);
    }
    void setFontSizePxAppliesInlineOverride() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        bridge.setFontSizePx(22);
        QVERIFY(btn.styleSheet().contains("font-size:22px"));
    }
    void coreExposesOperationStateChangedSignalAndReportOperationResult() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        QSignalSpy spy(&bridge.core(), &SControlCore::operationStateChanged);
        bridge.setOperationHandler([&]{ bridge.reportOperationResult(true); });
        bridge.triggerOperation();
        QCOMPARE(int(bridge.operationState()), int(OperationState::Success));
        QCOMPARE(spy.count(), 2); // Idle->Busy, Busy->Success
    }
    void resetOperationStateEarlyFromFailure() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        bridge.setOperationResetDelayMs(10000); // 足够长，确保下面的复位不是自动回退触发的
        bridge.setOperationHandler([&]{ bridge.reportOperationResult(false); });
        bridge.triggerOperation();
        QCOMPARE(int(bridge.operationState()), int(OperationState::Failure));
        bridge.resetOperationState();
        QCOMPARE(int(bridge.operationState()), int(OperationState::Idle));
    }
    void timeoutWithoutReportBecomesFailureViaBridge() {
        QPushButton btn;
        SControlBridge bridge(&btn);
        bridge.setOperationTimeoutMs(30);
        bridge.setOperationHandler([]{});
        bridge.triggerOperation();
        QCOMPARE(int(bridge.operationState()), int(OperationState::Busy));
        QTRY_COMPARE(int(bridge.operationState()), int(OperationState::Failure));
    }
    void attachReturnsNonNullAndAutoDestroysWithWidget() {
        auto* widget = new QPushButton;
        SControlBridge* bridge = slabelAttach(widget);
        QVERIFY(bridge != nullptr);
        QSignalSpy spy(bridge, &QObject::destroyed);
        delete widget;
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestControlBridge)
#include "test_control_bridge.moc"
