#include <QtTest>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QSlider>
#include <QProgressBar>
#include <QGroupBox>
#include <QTableView>
#include <QTreeView>
#include <QListView>
#include <QTabWidget>
#include <QDialog>
#include "slabel/SControl.h"

class TestControls : public QObject {
    Q_OBJECT
private slots:
    void allConstructAndSupportOverride() {
        SControl<QLabel> l; l.setThemeOverride("color", "#111"); QVERIFY(l.styleSheet().contains("#111"));
        SControl<QLineEdit> e; e.setThemeOverride("color", "#222"); QVERIFY(e.styleSheet().contains("#222"));
        SControl<QComboBox> c; QVERIFY(c.asWidget() == &c);
        SControl<QCheckBox> cb; cb.setThemeOverride("color", "#333"); QVERIFY(cb.styleSheet().contains("#333"));
        SControl<QRadioButton> rb; QVERIFY(rb.asWidget() == &rb);
        SControl<QSpinBox> sp; sp.setValue(5); QCOMPARE(sp.value(), 5);
        SControl<QSlider> sl; QVERIFY(sl.asWidget() == &sl);
        SControl<QProgressBar> pb; QVERIFY(pb.asWidget() == &pb);
        SControl<QGroupBox> gb; QVERIFY(gb.asWidget() == &gb);
        SControl<QTableView> tv; QVERIFY(tv.asWidget() == &tv);
        SControl<QTreeView> tr; QVERIFY(tr.asWidget() == &tr);
        SControl<QListView> lv; QVERIFY(lv.asWidget() == &lv);
        SControl<QTabWidget> tab; QVERIFY(tab.asWidget() == &tab);
        SControl<QDialog> dlg; QVERIFY(dlg.asWidget() == &dlg);
    }
};

QTEST_MAIN(TestControls)
#include "test_controls.moc"
