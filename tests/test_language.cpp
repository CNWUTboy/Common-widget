#include <QtTest>
#include <QCoreApplication>
#include <QPushButton>
#include <QLabel>
#include "slabel/SControl.h"
#include "slabel/LanguageManager.h"

class TestLanguage : public QObject {
    Q_OBJECT
private slots:
    void retranslateReappliesSourceText() {
        SControl<QPushButton> btn;
        btn.setTextTr("Save");           // 记住源串
        QCOMPARE(btn.text(), QString("Save"));
        // 没有加载 .qm 时 tr 返回源串，retranslate 后仍为 Save
        btn.retranslate();
        QCOMPARE(btn.text(), QString("Save"));
    }
    void unregisteredLanguageFails() {
        QVERIFY(!LanguageManager::instance().setLanguage("no_such_lang"));
    }
    void badPathFailsAndKeepsState() {
        auto& lm = LanguageManager::instance();
        const QString before = lm.currentLanguage();
        lm.registerLanguage("broken", "does_not_exist.qm");
        QVERIFY(!lm.setLanguage("broken"));        // load 失败返回 false
        QCOMPARE(lm.currentLanguage(), before);    // 状态未被破坏
    }
    void changeEventDispatchesToRetranslate() {
        // 验证 SControl<Base> 通用 changeEvent 转发：不直接调用 retranslate()，
        // 而是模拟 Qt 语言切换事件，证明裸 SControl<QLabel> 无需子类也能自动重译。
        SControl<QLabel> lbl;
        lbl.setTextTr("Save");
        QCOMPARE(lbl.text(), QString("Save"));
        lbl.setText(QStringLiteral("mutated")); // 人为改动，确认事件触发后被 retranslate 覆盖回源译文
        QEvent ev(QEvent::LanguageChange);
        QCoreApplication::sendEvent(&lbl, &ev);
        QCOMPARE(lbl.text(), QString("Save"));
    }
};

QTEST_MAIN(TestLanguage)
#include "test_language.moc"
