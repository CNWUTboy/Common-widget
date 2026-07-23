#include <QtTest>
#include <QFile>
#include <QPushButton>
#include "slabel/SControl.h"

class TestFontTokens : public QObject {
    Q_OBJECT
private slots:
    void primaryButtonRuleUsesDistinctFontSizeToken() {
        QFile f(QStringLiteral(THEME_DIR "/default.qss"));
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString text = QString::fromUtf8(f.readAll());
        const int idx = text.indexOf(QStringLiteral("*[slabelRole=\"primaryButton\"]"));
        QVERIFY(idx >= 0);
        const int end = text.indexOf(QChar('}'), idx);
        QVERIFY(end > idx);
        const QString rule = text.mid(idx, end - idx);
        // 角色规则必须有自己独立的字号 token（不是复用 @fontSizeBase）
        QVERIFY(rule.contains(QStringLiteral("font-size:@fontSizeButton")));
    }
    void setFontSizePxAppliesInlineOverride() {
        SControl<QPushButton> btn;
        btn.setFontSizePx(20);
        QVERIFY(btn.styleSheet().contains(QStringLiteral("font-size:20px")));
    }
};

QTEST_MAIN(TestFontTokens)
#include "test_font_tokens.moc"
