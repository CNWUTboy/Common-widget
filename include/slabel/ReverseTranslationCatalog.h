#pragma once
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include "slabel/SGlobal.h"

// 源串反查目录：解析 Qt Linguist 的 .ts 文件，建立
//   语言 -> (源串 -> {去重后的候选译文})
// 供全局重译引擎在"按控件类继承链解析 context 未命中"时兜底：
// 当某控件文字的 tr() context 位于外层类（不在控件类链上）时，
// 无法用 QTranslator::translate(className,...) 命中，改用本目录按源串反查。
// 若同一源串在不同 context 下有不同译文（歧义），translationsFor 返回多项，
// 由调用方决定不猜（保留源串）。
class SLABEL_EXPORT ReverseTranslationCatalog {
public:
    // 解析一个语言的 .ts 文件并并入目录（可多次调用累加）。成功返回 true。
    bool addLanguageFile(const QString& language, const QString& tsPath);

    // 该语言下按源串正查：返回去重后的候选译文（0 个=无；1 个=可用；>1=歧义）
    QStringList translationsFor(const QString& language, const QString& source) const;

    // 该语言下按译文反查源串：用于"切换后才创建、构造时已带该语言译文"的控件，
    // 先由译文回推源串，再正向翻到目标语言（0 个=无；1 个=可用；>1=歧义）
    QStringList sourcesFor(const QString& language, const QString& translation) const;

    bool hasLanguage(const QString& language) const { return m_fwd.contains(language); }
    void clear() { m_fwd.clear(); m_rev.clear(); }

private:
    // language -> (source -> set<translation>)
    QHash<QString, QHash<QString, QSet<QString>>> m_fwd;
    // language -> (translation -> set<source>)
    QHash<QString, QHash<QString, QSet<QString>>> m_rev;
};
