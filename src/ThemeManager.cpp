#include "slabel/ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QRegularExpression>

ThemeManager& ThemeManager::instance() {
    static ThemeManager s;
    return s;
}

QHash<QString, QString> ThemeManager::parseVariables(const QString& qss) {
    QHash<QString, QString> tokens;
    // 找首个块注释作为变量表
    QRegularExpression header(QStringLiteral("/\\*(.*?)\\*/"),
                             QRegularExpression::DotMatchesEverythingOption);
    auto m = header.match(qss);
    if (!m.hasMatch())
        return tokens;
    QRegularExpression var(QStringLiteral("@(\\w+)\\s*:\\s*(#[0-9A-Fa-f]+|[\\w]+)"));
    auto it = var.globalMatch(m.captured(1));
    while (it.hasNext()) {
        auto vm = it.next();
        tokens.insert(vm.captured(1), vm.captured(2)); // key 不含 @
    }
    return tokens;
}

QString ThemeManager::substituteVariables(const QString& qss) {
    const auto tokens = parseVariables(qss);
    QString out = qss;
    for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
        out.replace(QStringLiteral("@") + it.key(), it.value());
    return out;
}

void ThemeManager::registerTheme(const QString& name, const QString& qssPath) {
    m_themePaths.insert(name, qssPath);
}

bool ThemeManager::setTheme(const QString& name) {
    if (!m_themePaths.contains(name))
        return false;
    QFile f(m_themePaths.value(name));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QString raw = QString::fromUtf8(f.readAll());
    m_tokens = parseVariables(raw);               // 先存 token 表供自绘控件查询
    const QString qss = substituteVariables(raw);
    if (auto* app = qobject_cast<QApplication*>(QApplication::instance()))
        app->setStyleSheet(qss);
    m_current = name;
    emit themeChanged(name);
    return true;
}
