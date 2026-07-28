#pragma once
/**
 * @file ReverseTranslationCatalog.h
 * @brief 源串反查目录：解析 Qt Linguist 的 .ts 文件，建立源串<->译文的正反查映射，供重译引擎兜底。
 */
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include "slabel/SGlobal.h"

/**
 * @brief 源串反查目录：解析 Qt Linguist 的 .ts 文件，建立
 *        语言 -> (源串 -> {去重后的候选译文}) 及其反向映射。
 *
 * 供全局重译引擎在"按控件类继承链解析 context 未命中"时兜底：当某控件文字的
 * tr() context 位于外层类（不在控件类链上）时，无法用 QTranslator::translate(className,...)
 * 命中，改用本目录按源串反查。若同一源串在不同 context 下有不同译文（歧义），
 * translationsFor 返回多项，由调用方决定不猜（保留源串）。
 */
class SLABEL_EXPORT ReverseTranslationCatalog {
public:
    /**
     * @brief 解析一个语言的 .ts 文件并并入目录（可多次调用累加）。
     * @param language 语言名。
     * @param tsPath .ts 文件路径。
     * @return 成功返回 true（解析无错误）。
     */
    bool addLanguageFile(const QString& language, const QString& tsPath);

    /**
     * @brief 该语言下按源串正查候选译文。
     * @param language 语言名。
     * @param source 源串。
     * @return 去重后的候选译文（0 个=无；1 个=可用；>1=歧义）。
     */
    QStringList translationsFor(const QString& language, const QString& source) const;

    /**
     * @brief 该语言下按译文反查源串。
     * @param language 语言名。
     * @param translation 译文。
     * @return 去重后的候选源串（0 个=无；1 个=可用；>1=歧义）。
     * @note 用于"切换后才创建、构造时已带该语言译文"的控件：先由译文回推源串，再正向翻到目标语言。
     */
    QStringList sourcesFor(const QString& language, const QString& translation) const;

    /**
     * @brief 查询目录中是否已包含某语言。
     * @param language 语言名。
     * @return 已包含返回 true。
     */
    bool hasLanguage(const QString& language) const { return m_fwd.contains(language); }
    /**
     * @brief 清空目录（正反查映射全部移除）。
     */
    void clear() { m_fwd.clear(); m_rev.clear(); }

private:
    // language -> (source -> set<translation>)
    QHash<QString, QHash<QString, QSet<QString>>> m_fwd;
    // language -> (translation -> set<source>)
    QHash<QString, QHash<QString, QSet<QString>>> m_rev;
};
