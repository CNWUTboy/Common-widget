#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>
#include <functional>
#include "slabel/SGlobal.h"

class QWidget;
class QMetaObject;

// 一个可翻译文本"槽"：控件上某处可读写的一段文字。
// id 用于同一控件内区分多个槽（如 "text"/"title"/"item#3"/"tab#1"）。
struct TextSlot {
    QByteArray id;
    std::function<QString(QWidget*)> get;
    std::function<void(QWidget*, const QString&)> set;
};

// 一个控件类型的访问器：对该 metaObject 及其所有派生类生效。
// enumerate 在运行时按控件当前状态枚举槽（如下拉框/选项卡的项数运行时才知道）。
struct TypeAccessor {
    const QMetaObject* type = nullptr;
    std::function<QVector<TextSlot>(QWidget*)> enumerate;
};

// 可扩展的文本访问器登记表：按控件类型注册 getter/setter。
// slotsFor(w) 收集控件基类链上所有命中类型的槽并按 id 去重。
// 新增受支持的控件类型只需注册一个 TypeAccessor，不必改重译引擎。
class SLABEL_EXPORT TextAccessorRegistry {
public:
    static TextAccessorRegistry& instance();

    void registerType(TypeAccessor accessor);
    // 收集 w 上全部可翻译文本槽（跨其继承链命中的所有类型），按 id 去重
    QVector<TextSlot> slotsFor(QWidget* w) const;

    // 注册 v1 内置支持的控件类型（幂等；instance() 首次创建时自动调用）
    void registerBuiltinTypes();

private:
    TextAccessorRegistry() = default;
    QVector<TypeAccessor> m_accessors;
    bool m_builtinDone = false;
};
