#pragma once
/**
 * @file GlobalUiManager.h
 * @brief 全局零接入 UI 管理器：一次 install() 即可对全应用统一切换主题与语言。
 *
 * 主题委托复用 ThemeManager（qApp 全局 QSS）；语言走 GlobalRetranslator（整树重译）。
 */
#include <QObject>
#include <QString>
#include "slabel/SGlobal.h"

class QApplication;
class QWidget;
class GlobalRetranslator;

/**
 * @brief install() 的配置项。
 */
struct GlobalUiOptions {
    QString themeDir;              ///< 扫描其中 *.qss，按文件名注册为主题
    QString languageDir;           ///< 扫描其中 *.qm，按文件名注册为语言
    QString translationSourceDir;  ///< 可选：*.ts 目录，用于建立反查目录（外层类文字兜底）
    QString sourceLanguage = QStringLiteral("src"); ///< 源语言标识（构造期约定为此语言）
    QString initialTheme;          ///< 可选：install 后立即应用
    QString initialLanguage;       ///< 可选：若 != sourceLanguage，首帧前整树重译到此语言
};

/**
 * @brief 全局零接入 UI 管理器：使用方只需在 main() 中 install() 一次，
 *        即可对全应用未修改的原生/派生控件统一切换主题与语言。
 *
 * 主题委托复用 ThemeManager（qApp 全局 QSS）；语言走 GlobalRetranslator（整树重译）。
 */
class SLABEL_EXPORT GlobalUiManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 获取全局单例。
     * @return GlobalUiManager 单例引用。
     */
    static GlobalUiManager& instance();

    /**
     * @brief 安装全局 UI 管理：扫描注册主题/语言、安装事件过滤器、应用初始主题与语言。
     * @param app 应用对象。
     * @param options 安装配置项。
     * @return 成功返回 true；若已安装过则返回 false。
     * @note 契约：应在 QApplication 创建后、任何 UI 构造之前最先调用一次。
     */
    static bool install(QApplication& app, const GlobalUiOptions& options);

    /**
     * @brief 切换当前主题。
     * @param name 主题名（对应注册的 *.qss 文件名）。
     * @return 成功返回 true。
     */
    bool setTheme(const QString& name);
    /**
     * @brief 切换当前语言并整树重译。
     * @param name 语言名（对应注册的 *.qm 文件名，或源语言标识）。
     * @return 成功返回 true。
     */
    bool setLanguage(const QString& name);
    /**
     * @brief 查询当前主题名。
     * @return 当前主题名。
     */
    QString currentTheme() const;
    /**
     * @brief 查询当前语言名。
     * @return 当前语言名。
     */
    QString currentLanguage() const;

    /**
     * @brief 逃生口：使用方运行时改过某控件文字后，重新采集其源键。
     * @param w 需重新采集的控件。
     */
    void refreshSource(QWidget* w);

signals:
    /**
     * @brief 主题切换成功后发射。
     * @param name 新主题名。
     */
    void themeChanged(const QString& name);
    /**
     * @brief 语言切换成功后发射。
     * @param name 新语言名。
     */
    void languageChanged(const QString& name);

private:
    /**
     * @brief 私有构造：创建内部重译器。单例经 instance() 获取。
     */
    GlobalUiManager();
    GlobalRetranslator* m_retranslator = nullptr;
    bool m_installed = false;
};
