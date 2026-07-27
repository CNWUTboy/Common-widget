#pragma once
#include <QObject>
#include <QHash>
#include <QVector>
#include <QPointer>
#include <QString>
#include <QByteArray>
#include <QTranslator>
#include "slabel/SGlobal.h"
#include "slabel/TextAccessorRegistry.h"
#include "slabel/ReverseTranslationCatalog.h"

class QApplication;
class QWidget;
class QAction;
class QAbstractItemView;
class QEvent;

// 零接入全局重译引擎：作为 qApp 的全局事件过滤器，在每个控件出现时捕获其
// 文本作为"源键"，切换语言时遍历注册表整树回填；对 model/view 触发刷新。
// 不要求控件覆写 changeEvent，不修改控件代码。
class SLABEL_EXPORT GlobalRetranslator : public QObject {
    Q_OBJECT
public:
    explicit GlobalRetranslator(QObject* parent = nullptr);

    void setSourceLanguage(const QString& lang) { m_sourceLang = lang; if (m_current.isEmpty()) m_current = lang; }
    QString sourceLanguage() const { return m_sourceLang; }

    // 登记一个语言：name + .qm 路径（用于 QTranslator），可选 .ts 路径（用于反查目录兜底）
    void addLanguage(const QString& name, const QString& qmPath, const QString& tsPath = QString());

    // 安装为全局事件过滤器（应在任何 UI 构造前调用）
    void installOn(QApplication& app);

    // 切换当前语言并整树重译。切到源语言=卸载翻译器、恢复源文。成功返回 true。
    bool setLanguage(const QString& name);
    QString currentLanguage() const { return m_current; }

    // 使用方运行时改过某控件文字后，主动重采其源键（丢弃旧捕获重新登记）
    void refreshSource(QWidget* w);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    struct CapturedSlot { TextSlot slot; QString captured; QString capLang; };
    struct WidgetEntry { QPointer<QWidget> w; QVector<CapturedSlot> caps; };

    void captureWidget(QWidget* w);
    void captureAction(QAction* a);
    void retranslateAll();
    void refreshViews();
    bool hasSlotId(const WidgetEntry& e, const QByteArray& id) const;

    // 把"捕获文本(在 capLang 下) + 控件类型"解析为目标语言译文；无法解析则原样返回
    QString resolve(const QString& captured, const QString& capLang, const QMetaObject* mo) const;
    // 正向：源串 -> 当前目标语言译文（先按类继承链试 context，再反查目录兜底）
    QString forward(const QString& source, const QMetaObject* mo) const;
    // 反向：capLang 下的译文 -> 源串（用于切换后才创建的控件）
    QString toSource(const QString& captured, const QString& capLang) const;

    QString m_sourceLang = QStringLiteral("src");
    QString m_current;
    QHash<QString, QString> m_qmPaths;      // name -> qm 路径
    QTranslator m_translator;               // 当前安装的翻译器
    ReverseTranslationCatalog m_catalog;    // 反查/兜底目录

    QHash<QObject*, WidgetEntry> m_registry;       // 控件文本注册表
    QHash<QObject*, QString> m_actions;            // QAction -> 捕获源键（简化：假定构造期=源语言）
    QHash<QObject*, QString> m_actionLang;         // QAction -> capLang
    QVector<QPointer<QAbstractItemView>> m_views;  // 需刷新的 model/view

    bool m_translating = false;             // 回填期间抑制事件过滤器自捕获
};
