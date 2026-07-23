#include <QtTest>
#include "slabel/SGlobal.h"

class TestVersion : public QObject {
    Q_OBJECT
private slots:
    void versionStringIsNonEmpty() {
        const char* v = slabelVersion();
        QVERIFY(v != nullptr);
        QVERIFY(!QString::fromUtf8(v).isEmpty());
    }
};

QTEST_MAIN(TestVersion)
#include "test_version.moc"
