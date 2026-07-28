/**
 * @file ReverseTranslationCatalog.cpp
 * @brief ReverseTranslationCatalog 的实现：用 QXmlStreamReader 解析 .ts 文件并构建源串<->译文正反查映射。
 */
#include "slabel/ReverseTranslationCatalog.h"

#include <QFile>
#include <QXmlStreamReader>

bool ReverseTranslationCatalog::addLanguageFile(const QString& language, const QString& tsPath) {
    QFile f(tsPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    auto& fwd = m_fwd[language];
    auto& rev = m_rev[language];
    QXmlStreamReader xml(&f);

    QString curSource;
    QString curTranslation;
    bool curUnfinished = false;
    bool inMessage = false;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QStringRef name = xml.name();
            if (name == QLatin1String("message")) {
                inMessage = true;
                curSource.clear();
                curTranslation.clear();
                curUnfinished = false;
            } else if (inMessage && name == QLatin1String("source")) {
                curSource = xml.readElementText();
            } else if (inMessage && name == QLatin1String("translation")) {
                curUnfinished =
                    xml.attributes().value(QLatin1String("type")) == QLatin1String("unfinished");
                curTranslation = xml.readElementText();
            }
        } else if (xml.isEndElement() && xml.name() == QLatin1String("message")) {
            inMessage = false;
            if (!curSource.isEmpty() && !curTranslation.isEmpty() && !curUnfinished) {
                fwd[curSource].insert(curTranslation);
                rev[curTranslation].insert(curSource);
            }
        }
    }

    return !xml.hasError();
}

QStringList ReverseTranslationCatalog::translationsFor(const QString& language,
                                                       const QString& source) const {
    auto langIt = m_fwd.constFind(language);
    if (langIt == m_fwd.constEnd())
        return {};
    auto it = langIt->constFind(source);
    if (it == langIt->constEnd())
        return {};
    return QStringList(it->values());
}

QStringList ReverseTranslationCatalog::sourcesFor(const QString& language,
                                                  const QString& translation) const {
    auto langIt = m_rev.constFind(language);
    if (langIt == m_rev.constEnd())
        return {};
    auto it = langIt->constFind(translation);
    if (it == langIt->constEnd())
        return {};
    return QStringList(it->values());
}
