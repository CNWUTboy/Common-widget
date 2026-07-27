#pragma once
#include <QObject>
#include <QString>
#include "slabel/SGlobal.h"

class QApplication;
class QWidget;
class GlobalRetranslator;

// install() 的配置项
struct GlobalUiOptions {
    QString themeDir;              // 扫描其中 *.qss，按文件名注册为主题
    QString languageDir;           // 扫描其中 *.qm，按文件名注册为语言
    QString translationSourceDir;  // 可选：*.ts 目录，用于建立反查目录（外层类文字兜底）
    QString sourceLanguage = QStringLiteral("src"); // 源语言标识（构造期约定为此语言）
    QString initialTheme;          // 可选：install 后立即应用
    QString initialLanguage;       // 可选：若 != sourceLanguage，首帧前整树重译到此语言
};

// 全局零接入 UI 管理器：使用方只需在 main() 中 install() 一次，
// 即可对全应用未修改的原生/派生控件统一切换主题与语言。
// 主题委托复用 ThemeManager（qApp 全局 QSS）；语言走 GlobalRetranslator（整树重译）。
class SLABEL_EXPORT GlobalUiManager : public QObject {
    Q_OBJECT
public:
    static GlobalUiManager& instance();

    // 契约：应在 QApplication 创建后、任何 UI 构造之前最先调用一次。重复调用返回 false。
    static bool install(QApplication& app, const GlobalUiOptions& options);

    bool setTheme(const QString& name);
    bool setLanguage(const QString& name);
    QString currentTheme() const;
    QString currentLanguage() const;

    // 逃生口：使用方运行时改过某控件文字后，重新采集其源键
    void refreshSource(QWidget* w);

signals:
    void themeChanged(const QString& name);
    void languageChanged(const QString& name);

private:
    GlobalUiManager();
    GlobalRetranslator* m_retranslator = nullptr;
    bool m_installed = false;
};
