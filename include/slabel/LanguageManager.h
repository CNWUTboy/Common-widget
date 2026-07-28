#pragma once

/**
 * @file LanguageManager.h
 * @brief 语言管理器 LanguageManager 定义。
 *
 * 负责翻译文件的注册与切换，并统一通知已注册控件重译文案。
 */

#include <QObject>
#include <QHash>
#include <QSet>
#include <QTranslator>
#include "slabel/SGlobal.h"
#include "slabel/ISControl.h"

/**
 * @brief 语言管理器（单例）。
 *
 * 负责加载/切换 QTranslator，并通知所有已注册控件重译文案。
 */
class SLABEL_EXPORT LanguageManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 获取单例实例。
     * @return 语言管理器的全局唯一引用。
     */
    static LanguageManager& instance();

    /**
     * @brief 注册一种语言及其翻译文件路径。
     * @param name 语言名称。
     * @param qmPath 对应 .qm 翻译文件路径。
     */
    void registerLanguage(const QString& name, const QString& qmPath);
    /**
     * @brief 切换到指定语言。
     * @param name 已注册的语言名称。
     * @return 切换成功返回 true；语言未注册或翻译文件载入失败返回 false。
     */
    bool setLanguage(const QString& name);
    /**
     * @brief 获取当前语言名称。
     * @return 当前语言名称。
     */
    QString currentLanguage() const { return m_current; }

    /**
     * @brief 注册控件以接收语言切换重译通知。
     * @param c 待注册的控件接口指针。
     */
    void registerControl(ISControl* c) { m_controls.insert(c); }
    /**
     * @brief 注销控件，不再接收语言切换通知。
     * @param c 待注销的控件接口指针。
     */
    void unregisterControl(ISControl* c) { m_controls.remove(c); }

signals:
    /**
     * @brief 语言切换成功后发射。
     * @param name 切换后的语言名称。
     */
    void languageChanged(const QString& name);

private:
    LanguageManager() = default;
    QHash<QString, QString> m_qmPaths;
    QString m_current;
    QTranslator m_translator;
    QSet<ISControl*> m_controls;
};
