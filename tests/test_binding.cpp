#include <QtTest>
#include "slabel/BindingEngine.h"
#include "slabel/SBindable.h"

class TestBinding : public QObject {
    Q_OBJECT
private slots:
    void twoWaySyncsBothDirections() {
        SBindableObject a, b;
        a.setValue(1);
        BindingEngine::bind(&a, "value", &b, "value");
        // bind 时初始同步 a→b
        QCOMPARE(b.value().toInt(), 1);
        a.setValue(42);
        QCOMPARE(b.value().toInt(), 42);
        b.setValue(7);
        QCOMPARE(a.value().toInt(), 7);   // 反向也同步
    }
    void noInfiniteLoop() {
        SBindableObject a, b;
        BindingEngine::bind(&a, "value", &b, "value");
        a.setValue(5);                    // 不应死循环
        QCOMPARE(a.value().toInt(), 5);
        QCOMPARE(b.value().toInt(), 5);
    }
    void observeInvokesCallback() {
        SBindableObject a;
        int seen = -1;
        BindingEngine::observe(&a, "value", [&](const QVariant& v){ seen = v.toInt(); });
        a.setValue(9);
        QCOMPARE(seen, 9);
    }
    void unbindStopsSync() {
        SBindableObject a, b;
        BindingEngine::bind(&a, "value", &b, "value");
        BindingEngine::unbind(&a, "value");
        a.setValue(3);
        QVERIFY(b.value().toInt() != 3 || b.value().isNull());
    }
    void rebindReplacesOldBinding() {
        SBindableObject a, b1, b2;
        BindingEngine::bind(&a, "value", &b1, "value");
        BindingEngine::bind(&a, "value", &b2, "value");   // 覆盖同一 key
        a.setValue(99);
        QCOMPARE(b2.value().toInt(), 99);                  // 新绑定生效
        QVERIFY(b1.value().toInt() != 99);                 // 旧绑定已被清理，不再同步
    }
    void peerDestructionStopsSyncSafely() {
        SBindableObject a;
        auto* b = new SBindableObject; // 堆上分配，便于提前销毁模拟对端先亡
        BindingEngine::bind(&a, "value", b, "value");
        delete b;                       // 销毁对端：Binding 应自动摘除自身
        a.setValue(123);                // 若仍持悬垂指针会在此 UAF/崩溃
        QCOMPARE(a.value().toInt(), 123);
    }
    void unbindFromWithinObserveCallbackDoesNotCrash() {
        SBindableObject a;
        BindingEngine::observe(&a, "value", [&a](const QVariant&){
            BindingEngine::unbind(&a, "value"); // 回调内对同一 key 自解绑（发射途中）
        });
        a.setValue(1); // 不应崩溃（delete-during-emission 已改为 deleteLater）
        QCOMPARE(a.value().toInt(), 1);
    }
};

QTEST_MAIN(TestBinding)
#include "test_binding.moc"
