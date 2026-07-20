#include "slabel/LanguageManager.h"
#include <QApplication>

LanguageManager& LanguageManager::instance() {
    static LanguageManager s;
    return s;
}

void LanguageManager::registerLanguage(const QString& name, const QString& qmPath) {
    m_qmPaths.insert(name, qmPath);
}

bool LanguageManager::setLanguage(const QString& name) {
    if (!m_qmPaths.contains(name))
        return false;
    QApplication::removeTranslator(&m_translator);
    if (!m_translator.load(m_qmPaths.value(name)))
        return false;
    QApplication::installTranslator(&m_translator);
    m_current = name;
    // 通知所有注册控件重译（不遍历整棵树，只走注册表）
    for (ISControl* c : m_controls)
        c->retranslate();
    emit languageChanged(name);
    return true;
}
