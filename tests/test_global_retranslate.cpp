#include <QtTest>
#include <QApplication>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTabBar>
#include <QAbstractListModel>
#include <QTableView>
#include "slabel/GlobalUiManager.h"

// 模拟"基于原生派生、自己在构造里 tr() 设文字、但不覆写 changeEvent"的控件。
// Q_OBJECT 使其 tr() 的 context = "MyButton"，位于控件类继承链上。
class MyButton : public QPushButton {
    Q_OBJECT
public:
    explicit MyButton(QWidget* p = nullptr) : QPushButton(p) { setText(tr("Go")); }
};

// 模拟"data() 用 tr() 返回、可重取"的 model（context = "CellModel"）。
class CellModel : public QAbstractListModel {
    Q_OBJECT
public:
    int rowCount(const QModelIndex& = QModelIndex()) const override { return 1; }
    QVariant data(const QModelIndex& idx, int role) const override {
        if (role == Qt::DisplayRole && idx.row() == 0) return tr("Cell");
        return QVariant();
    }
};

static void pump(QWidget* w) { w->show(); QApplication::processEvents(); }

class TestGlobalRetranslate : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        GlobalUiOptions o;
        o.sourceLanguage = "src";
        o.languageDir = GLOBAL_QM_DIR;          // 含 zz.qm
        o.translationSourceDir = GLOBAL_TS_DIR;  // 含 zz.ts（反查目录）
        QVERIFY(GlobalUiManager::install(*qApp, o));
    }
    void init() { GlobalUiManager::instance().setLanguage("src"); } // 每例复位到源语言

    // 1. 原生控件、文字由外层类 tr() 设置 → 走反查目录
    void nativeButtonViaReverseCatalog() {
        QPushButton b; b.setText(tr("Save"));    // context = 本测试类，不在按钮类链上
        pump(&b);
        QVERIFY(GlobalUiManager::instance().setLanguage("zz"));
        QCOMPARE(b.text(), QString("SaveZZ"));
        QVERIFY(GlobalUiManager::instance().setLanguage("src"));
        QCOMPARE(b.text(), QString("Save"));     // 切回源语言恢复源文
    }

    // 2. 派生控件自设文字 → 走类继承链 context 命中
    void derivedButtonViaContextChain() {
        MyButton mb;                              // 构造里 tr("Go")，context = MyButton
        pump(&mb);
        QVERIFY(GlobalUiManager::instance().setLanguage("zz"));
        QCOMPARE(mb.text(), QString("GoZZ"));
    }

    // 3. 同源多 context 不同译 → 歧义，保留源串不乱翻
    void ambiguousKeepsSource() {
        QLabel l; l.setText(tr("Color"));         // zz.ts 里 AmbA/AmbB 都有 Color
        pump(&l);
        QVERIFY(GlobalUiManager::instance().setLanguage("zz"));
        QCOMPARE(l.text(), QString("Color"));
    }

    // 4. 切换后才创建的控件：立即本地化；再切回源语言经"译文→源串"还原
    void lateCreatedWidgetLocalized() {
        QVERIFY(GlobalUiManager::instance().setLanguage("zz"));
        MyButton mb;                              // zz 已装，构造时 tr("Go") 直接得 GoZZ
        pump(&mb);
        QCOMPARE(mb.text(), QString("GoZZ"));
        QVERIFY(GlobalUiManager::instance().setLanguage("src"));
        QCOMPARE(mb.text(), QString("Go"));       // 反向还原源串
    }

    // 5. 多项文本：下拉框各 item + 选项卡各 tab
    void multiItemTexts() {
        QComboBox c; c.addItem(tr("Opt1")); c.addItem(tr("Opt2"));
        QTabBar t; t.addTab(tr("TabA")); t.addTab(tr("TabB"));
        pump(&c); pump(&t);
        QVERIFY(GlobalUiManager::instance().setLanguage("zz"));
        QCOMPARE(c.itemText(0), QString("Opt1ZZ"));
        QCOMPARE(c.itemText(1), QString("Opt2ZZ"));
        QCOMPARE(t.tabText(0), QString("TabAZZ"));
        QCOMPARE(t.tabText(1), QString("TabBZZ"));
    }

    // 6. 控件销毁后再切语言：QPointer/destroyed 清理，不崩溃
    void destroyedWidgetSafe() {
        auto* b = new QPushButton;
        b->setText(tr("Save"));
        pump(b);
        delete b;                                 // destroyed → 从注册表摘除
        QVERIFY(GlobalUiManager::instance().setLanguage("zz")); // 遍历不应崩
    }

    // 7. model/view：戳视图刷新，model 的 data() 内 tr() 按新语言重算
    void modelViewRefresh() {
        QTableView v; CellModel m; v.setModel(&m);
        pump(&v);
        QVERIFY(GlobalUiManager::instance().setLanguage("zz"));
        QCOMPARE(m.data(m.index(0, 0), Qt::DisplayRole).toString(), QString("CellZZ"));
    }

    // 8. 窗口标题
    void windowTitle() {
        QWidget w; w.setWindowTitle(tr("Hint"));
        pump(&w);
        QVERIFY(GlobalUiManager::instance().setLanguage("zz"));
        QCOMPARE(w.windowTitle(), QString("HintZZ"));
    }
};

QTEST_MAIN(TestGlobalRetranslate)
#include "test_global_retranslate.moc"
