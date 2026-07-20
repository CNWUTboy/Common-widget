#pragma once
#include <QObject>
#include <QHash>
#include <QSet>
#include <QColor>
#include "slabel/SGlobal.h"
#include "slabel/ISControl.h"

// 主题管理器：负责加载 QSS、变量替换（@key -> value）与主题切换，
// 并维护当前主题的语义色 token 表，供自绘控件查询。
class SLABEL_EXPORT ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    void registerTheme(const QString& name, const QString& qssPath);
    bool setTheme(const QString& name);
    QString currentTheme() const { return m_current; }

    void registerControl(ISControl* c) { m_controls.insert(c); }
    void unregisterControl(ISControl* c) { m_controls.remove(c); }

    // 解析首个 /* @k:v ... */ 头为 {k:v}
    static QHash<QString, QString> parseVariables(const QString& qss);
    // 用 parseVariables 把 @k 替换为 v
    static QString substituteVariables(const QString& qss);
    // 当前主题下的语义色（供自绘控件查询）；未定义返回无效 QColor
    QColor token(const QString& name) const { return QColor(m_tokens.value(name)); }

signals:
    void themeChanged(const QString& name);

private:
    ThemeManager() = default;
    QHash<QString, QString> m_themePaths;
    QString m_current;
    QSet<ISControl*> m_controls;
    QHash<QString, QString> m_tokens; // 当前主题的语义色表
};
