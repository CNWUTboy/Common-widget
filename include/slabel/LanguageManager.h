#pragma once
#include <QObject>
#include <QHash>
#include <QSet>
#include <QTranslator>
#include "slabel/SGlobal.h"
#include "slabel/ISControl.h"

// 语言管理器：单例，负责加载/切换 QTranslator，并通知所有已注册控件重译文案。
class SLABEL_EXPORT LanguageManager : public QObject {
    Q_OBJECT
public:
    static LanguageManager& instance();

    void registerLanguage(const QString& name, const QString& qmPath);
    bool setLanguage(const QString& name);
    QString currentLanguage() const { return m_current; }

    void registerControl(ISControl* c) { m_controls.insert(c); }
    void unregisterControl(ISControl* c) { m_controls.remove(c); }

signals:
    void languageChanged(const QString& name);

private:
    LanguageManager() = default;
    QHash<QString, QString> m_qmPaths;
    QString m_current;
    QTranslator m_translator;
    QSet<ISControl*> m_controls;
};
