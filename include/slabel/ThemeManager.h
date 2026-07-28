#pragma once

/**
 * @file ThemeManager.h
 * @brief 主题管理器 ThemeManager 定义。
 *
 * 负责 QSS 加载、变量替换与主题切换，并维护语义色 token 表供自绘控件查询。
 */

#include <QObject>
#include <QHash>
#include <QSet>
#include <QColor>
#include "slabel/SGlobal.h"
#include "slabel/ISControl.h"

/**
 * @brief 主题管理器（单例）。
 *
 * 负责加载 QSS、变量替换（@key -> value）与主题切换，并维护当前主题的语义色
 * token 表，供自绘控件查询。
 */
class SLABEL_EXPORT ThemeManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 获取单例实例。
     * @return 主题管理器的全局唯一引用。
     */
    static ThemeManager& instance();

    /**
     * @brief 注册一个主题及其 QSS 文件路径。
     * @param name 主题名称。
     * @param qssPath 对应 QSS 文件路径。
     */
    void registerTheme(const QString& name, const QString& qssPath);
    /**
     * @brief 切换到指定主题。
     * @param name 已注册的主题名称。
     * @return 切换成功返回 true；主题未注册或 QSS 文件读取失败返回 false。
     */
    bool setTheme(const QString& name);
    /**
     * @brief 获取当前主题名称。
     * @return 当前主题名称。
     */
    QString currentTheme() const { return m_current; }

    /**
     * @brief 注册控件以纳入主题统一管理。
     * @param c 待注册的控件接口指针。
     */
    void registerControl(ISControl* c) { m_controls.insert(c); }
    /**
     * @brief 注销控件，不再纳入主题管理。
     * @param c 待注销的控件接口指针。
     */
    void unregisterControl(ISControl* c) { m_controls.remove(c); }

    /**
     * @brief 解析 QSS 首个 /* @k:v ... *\/ 块注释头为变量表。
     * @param qss QSS 文本。
     * @return 解析得到的 {k:v} 变量表（key 不含 @）；无匹配时返回空表。
     */
    static QHash<QString, QString> parseVariables(const QString& qss);
    /**
     * @brief 用 parseVariables 得到的变量表把 QSS 中的 @k 替换为 v。
     * @param qss 含变量引用的 QSS 文本。
     * @return 替换后的 QSS 文本。
     */
    static QString substituteVariables(const QString& qss);
    /**
     * @brief 查询当前主题下某 token 的原始值。
     * @param name token 名称（颜色/字体/图标路径等）。
     * @return token 原始字符串值；未定义时返回空字符串。
     * @note 类型转换由调用方按需选择便捷方法（如 colorToken）或自行处理。
     */
    QString token(const QString& name) const { return m_tokens.value(name); }
    /**
     * @brief 以 QColor 形式查询当前主题下的颜色 token。
     * @param name token 名称。
     * @return 对应颜色；token 未定义时为无效 QColor。
     */
    QColor colorToken(const QString& name) const { return QColor(token(name)); }

signals:
    /**
     * @brief 主题切换成功后发射。
     * @param name 切换后的主题名称。
     */
    void themeChanged(const QString& name);

private:
    ThemeManager() = default;
    QHash<QString, QString> m_themePaths;
    QString m_current;
    QSet<ISControl*> m_controls;
    QHash<QString, QString> m_tokens; // 当前主题的语义色表
};
