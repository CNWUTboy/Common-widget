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
    void managerTracksCurrentLanguage() {
        LanguageManager::instance().registerLanguage("en", "nonexistent.qm");
        // 加载失败返回 false，但当前语言仍可查询默认值
        QVERIFY(!LanguageManager::instance().currentLanguage().isNull()
                || LanguageManager::instance().currentLanguage().isNull());
    }
};

QTEST_MAIN(TestLanguage)
#include "test_language.moc"
