#include <QtTest>
#include "slabel/SControlCore.h"

class TestControlCore : public QObject {
    Q_OBJECT
private slots:
    void emitsSignalOnNotify() {
        SControlCore core;
        QSignalSpy spy(&core, &SControlCore::bindablePropertyChanged);
        core.notifyChanged();
        QCOMPARE(spy.count(), 1);
    }
};

QTEST_MAIN(TestControlCore)
#include "test_controlcore.moc"
