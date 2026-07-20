#include <QtTest>
#include "slabel/SButton.h"
#include "slabel/LanguageManager.h"

class TestLanguage : public QObject {
    Q_OBJECT
private slots:
    void retranslateReappliesSourceText() {
        SButton btn;
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
};

QTEST_MAIN(TestLanguage)
#include "test_language.moc"
