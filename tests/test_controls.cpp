#include <QtTest>
#include "slabel/SLabel.h"
#include "slabel/SLineEdit.h"
#include "slabel/SComboBox.h"
#include "slabel/SCheckBox.h"
#include "slabel/SRadioButton.h"
#include "slabel/SSpinBox.h"
#include "slabel/SSlider.h"
#include "slabel/SProgressBar.h"
#include "slabel/SGroupBox.h"
#include "slabel/STableView.h"
#include "slabel/STreeView.h"
#include "slabel/SListView.h"
#include "slabel/STabWidget.h"
#include "slabel/SDialog.h"

class TestControls : public QObject {
    Q_OBJECT
private slots:
    void allConstructAndSupportOverride() {
        SLabel l; l.setThemeOverride("color", "#111"); QVERIFY(l.styleSheet().contains("#111"));
        SLineEdit e; e.setThemeOverride("color", "#222"); QVERIFY(e.styleSheet().contains("#222"));
        SComboBox c; QVERIFY(c.asWidget() == &c);
        SCheckBox cb; cb.setThemeOverride("color", "#333"); QVERIFY(cb.styleSheet().contains("#333"));
        SRadioButton rb; QVERIFY(rb.asWidget() == &rb);
        SSpinBox sp; sp.setValue(5); QCOMPARE(sp.value(), 5);
        SSlider sl; QVERIFY(sl.asWidget() == &sl);
        SProgressBar pb; QVERIFY(pb.asWidget() == &pb);
        SGroupBox gb; QVERIFY(gb.asWidget() == &gb);
        STableView tv; QVERIFY(tv.asWidget() == &tv);
        STreeView tr; QVERIFY(tr.asWidget() == &tr);
        SListView lv; QVERIFY(lv.asWidget() == &lv);
        STabWidget tab; QVERIFY(tab.asWidget() == &tab);
        SDialog dlg; QVERIFY(dlg.asWidget() == &dlg);
    }
};

QTEST_MAIN(TestControls)
#include "test_controls.moc"
