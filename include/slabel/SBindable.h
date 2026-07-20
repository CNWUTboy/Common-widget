#pragma once
#include <QObject>
#include <QVariant>
#include "slabel/SGlobal.h"

// 轻量可绑定数据宿主：业务对象可继承或直接使用
class SLABEL_EXPORT SBindableObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged)
public:
    explicit SBindableObject(QObject* parent = nullptr) : QObject(parent) {}
    QVariant value() const { return m_value; }
    void setValue(const QVariant& v) {
        if (m_value == v) return;
        m_value = v;
        emit valueChanged(m_value);
    }
signals:
    void valueChanged(const QVariant& value);
private:
    QVariant m_value;
};
