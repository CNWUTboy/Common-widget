#include <QtTest>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTabBar>
#include "slabel/TextAccessorRegistry.h"

static const TextSlot* findSlot(const QVector<TextSlot>& v, const QByteArray& id) {
    for (const TextSlot& s : v) if (s.id == id) return &s;
    return nullptr;
}

class TestTextAccessors : public QObject {
    Q_OBJECT
private slots:
    void buttonText() {
        QPushButton b; b.setText("Hi");
        auto sl = TextAccessorRegistry::instance().slotsFor(&b);
        const TextSlot* s = findSlot(sl, "text");
        QVERIFY(s);
        QCOMPARE(s->get(&b), QString("Hi"));
        s->set(&b, "Yo");
        QCOMPARE(b.text(), QString("Yo"));
    }

    void labelPlainTextOnly() {
        QLabel plain; plain.setText("Plain");
        QVERIFY(findSlot(TextAccessorRegistry::instance().slotsFor(&plain), "text"));
        QLabel rich; rich.setText("<b>Rich</b>");
        QVERIFY(!findSlot(TextAccessorRegistry::instance().slotsFor(&rich), "text")); // 富文本跳过
    }

    void groupBoxTitle() {
        QGroupBox g; g.setTitle("Grp");
        const TextSlot* s = findSlot(TextAccessorRegistry::instance().slotsFor(&g), "title");
        QVERIFY(s);
        QCOMPARE(s->get(&g), QString("Grp"));
    }

    void lineEditPlaceholder() {
        QLineEdit e; e.setPlaceholderText("PH");
        const TextSlot* s = findSlot(TextAccessorRegistry::instance().slotsFor(&e), "placeholder");
        QVERIFY(s);
        QCOMPARE(s->get(&e), QString("PH"));
    }

    void comboItems() {
        QComboBox c; c.addItem("A"); c.addItem("B");
        auto sl = TextAccessorRegistry::instance().slotsFor(&c);
        QVERIFY(findSlot(sl, "item#0"));
        const TextSlot* s1 = findSlot(sl, "item#1");
        QVERIFY(s1);
        QCOMPARE(s1->get(&c), QString("B"));
        s1->set(&c, "BB");
        QCOMPARE(c.itemText(1), QString("BB"));
    }

    void tabTexts() {
        QTabBar t; t.addTab("T0"); t.addTab("T1");
        auto sl = TextAccessorRegistry::instance().slotsFor(&t);
        QVERIFY(findSlot(sl, "tab#0"));
        const TextSlot* s1 = findSlot(sl, "tab#1");
        QVERIFY(s1);
        QCOMPARE(s1->get(&t), QString("T1"));
    }

    void windowTitleOnlyWhenSet() {
        QWidget noTitle;
        QVERIFY(!findSlot(TextAccessorRegistry::instance().slotsFor(&noTitle), "windowTitle"));
        QWidget titled; titled.setWindowTitle("W");
        QVERIFY(findSlot(TextAccessorRegistry::instance().slotsFor(&titled), "windowTitle"));
    }
};

QTEST_MAIN(TestTextAccessors)
#include "test_text_accessors.moc"
