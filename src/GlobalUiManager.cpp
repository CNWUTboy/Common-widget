/**
 * @file GlobalUiManager.cpp
 * @brief GlobalUiManager 的实现：主题/语言目录扫描注册、安装事件过滤器、切换主题与语言。
 */
#include "slabel/GlobalUiManager.h"
#include "slabel/GlobalRetranslator.h"
#include "slabel/ThemeManager.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

GlobalUiManager::GlobalUiManager() {
    m_retranslator = new GlobalRetranslator(this);
}

GlobalUiManager& GlobalUiManager::instance() {
    static GlobalUiManager s;
    return s;
}

bool GlobalUiManager::install(QApplication& app, const GlobalUiOptions& opt) {
    GlobalUiManager& self = instance();
    if (self.m_installed) {
        qWarning("GlobalUiManager::install 已调用过，忽略重复安装");
        return false;
    }
    self.m_installed = true;

    // ---- 主题：扫描 *.qss 注册（复用 ThemeManager 的全局 QSS 下发）----
    if (!opt.themeDir.isEmpty()) {
        QDir dir(opt.themeDir);
        for (const QFileInfo& fi : dir.entryInfoList(QStringList() << "*.qss", QDir::Files))
            ThemeManager::instance().registerTheme(fi.baseName(), fi.absoluteFilePath());
    }

    // ---- 语言：扫描 *.qm 注册；如有 *.ts 目录则建反查目录 ----
    self.m_retranslator->setSourceLanguage(opt.sourceLanguage);
    if (!opt.languageDir.isEmpty()) {
        QDir dir(opt.languageDir);
        for (const QFileInfo& fi : dir.entryInfoList(QStringList() << "*.qm", QDir::Files)) {
            QString ts;
            if (!opt.translationSourceDir.isEmpty()) {
                const QString cand = QDir(opt.translationSourceDir).filePath(fi.baseName() + ".ts");
                if (QFileInfo::exists(cand)) ts = cand;
            }
            self.m_retranslator->addLanguage(fi.baseName(), fi.absoluteFilePath(), ts);
        }
    }

    // ---- 安装全局事件过滤器（此时尚未安装任何翻译器，维持"构造期=源语言"契约）----
    self.m_retranslator->installOn(app);

    // ---- 初始主题 ----
    if (!opt.initialTheme.isEmpty())
        ThemeManager::instance().setTheme(opt.initialTheme);

    // ---- 初始语言：若非源语言，于首帧前（事件循环启动时）整树重译一次 ----
    if (!opt.initialLanguage.isEmpty() && opt.initialLanguage != opt.sourceLanguage) {
        const QString lang = opt.initialLanguage;
        QTimer::singleShot(0, &self, [&self, lang] { self.setLanguage(lang); });
    }
    return true;
}

bool GlobalUiManager::setTheme(const QString& name) {
    if (!ThemeManager::instance().setTheme(name)) return false;
    emit themeChanged(name);
    return true;
}

bool GlobalUiManager::setLanguage(const QString& name) {
    if (!m_retranslator->setLanguage(name)) return false;
    emit languageChanged(name);
    return true;
}

QString GlobalUiManager::currentTheme() const {
    return ThemeManager::instance().currentTheme();
}

QString GlobalUiManager::currentLanguage() const {
    return m_retranslator->currentLanguage();
}

void GlobalUiManager::refreshSource(QWidget* w) {
    m_retranslator->refreshSource(w);
}
