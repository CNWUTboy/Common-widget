#pragma once
/**
 * @file TextAccessorRegistry.h
 * @brief 可扩展的文本访问器登记表：按控件类型注册文本 getter/setter，供全局重译引擎读写。
 *
 * 新增受支持的控件类型只需注册一个 TypeAccessor，不必改重译引擎。
 */
#include <QByteArray>
#include <QString>
#include <QVector>
#include <functional>
#include "slabel/SGlobal.h"

class QWidget;
class QMetaObject;

/**
 * @brief 一个可翻译文本"槽"：控件上某处可读写的一段文字。
 *
 * id 用于同一控件内区分多个槽（如 "text"/"title"/"item#3"/"tab#1"）。
 */
struct TextSlot {
    QByteArray id;                                  ///< 槽标识，用于同一控件内区分多个槽
    std::function<QString(QWidget*)> get;           ///< 读取该槽当前文本
    std::function<void(QWidget*, const QString&)> set; ///< 写入该槽文本
};

/**
 * @brief 一个控件类型的访问器：对该 metaObject 及其所有派生类生效。
 *
 * enumerate 在运行时按控件当前状态枚举槽（如下拉框/选项卡的项数运行时才知道）。
 */
struct TypeAccessor {
    const QMetaObject* type = nullptr;              ///< 目标控件类型的静态元对象
    std::function<QVector<TextSlot>(QWidget*)> enumerate; ///< 运行时枚举该控件的全部文本槽
};

/**
 * @brief 可扩展的文本访问器登记表：按控件类型注册 getter/setter。
 *
 * slotsFor(w) 收集控件基类链上所有命中类型的槽并按 id 去重。
 * 新增受支持的控件类型只需注册一个 TypeAccessor，不必改重译引擎。
 */
class SLABEL_EXPORT TextAccessorRegistry {
public:
    /**
     * @brief 获取全局单例；首次创建时自动注册内置控件类型。
     * @return TextAccessorRegistry 单例引用。
     */
    static TextAccessorRegistry& instance();

    /**
     * @brief 注册一个控件类型访问器。
     * @param accessor 待注册的访问器（type 与 enumerate 均非空才生效）。
     */
    void registerType(TypeAccessor accessor);
    /**
     * @brief 收集 w 上全部可翻译文本槽（跨其继承链命中的所有类型），按 id 去重。
     * @param w 目标控件。
     * @return 去重后的文本槽集合。
     */
    QVector<TextSlot> slotsFor(QWidget* w) const;

    /**
     * @brief 注册 v1 内置支持的控件类型。
     * @note 幂等；instance() 首次创建时自动调用。
     */
    void registerBuiltinTypes();

private:
    /**
     * @brief 私有默认构造。单例经 instance() 获取。
     */
    TextAccessorRegistry() = default;
    QVector<TypeAccessor> m_accessors;
    bool m_builtinDone = false;
};
