#pragma once
/**
 * @file SBindable.h
 * @brief 轻量可绑定数据宿主 SBindableObject：暴露单个可读写、带变更通知的 value 属性。
 *
 * 常与 BindingEngine 配合，作为绑定/监听的一端。该能力已弃用。
 */
#include <QObject>
#include <QVariant>
#include "slabel/SGlobal.h"

/**
 * @brief 轻量可绑定数据宿主：业务对象可继承或直接使用。
 *
 * 通过 QVariant 属性 value 持有单个值，写入变化时发射 valueChanged。
 *
 * @deprecated 状态绑定能力已弃用，将在后续版本移除；请改用原生 Qt 信号/槽或属性绑定。
 */
class SLABEL_EXPORT SLABEL_DEPRECATED(
    "状态绑定能力已弃用，将在后续版本移除；请改用原生 Qt 信号/槽或属性绑定") SBindableObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
public:
    /**
     * @brief 构造可绑定对象。
     * @param parent 父对象，交由 Qt 父子链管理生命周期。
     */
    explicit SBindableObject(QObject* parent = nullptr) : QObject(parent) {}
    /**
     * @brief 读取当前值。
     * @return 当前持有的 QVariant 值。
     */
    QVariant value() const { return m_value; }
    /**
     * @brief 写入新值，仅在值发生变化时更新并发射 valueChanged。
     * @param v 待写入的新值。
     */
    void setValue(const QVariant& v) {
        if (m_value == v) return;
        m_value = v;
        emit valueChanged(m_value);
    }
signals:
    /**
     * @brief 当 value 被设置为不同的新值时发射。
     * @param value 变更后的当前值。
     */
    void valueChanged(const QVariant& value);
private:
    QVariant m_value;
};
