#pragma once
#include <QObject>
#include "slabel/SGlobal.h"

// 为非 QObject 的 CRTP 模板提供 signal 的小助手
class SLABEL_EXPORT SControlCore : public QObject {
    Q_OBJECT
public:
    explicit SControlCore(QObject* parent = nullptr) : QObject(parent) {}
    void notifyChanged() { emit bindablePropertyChanged(); }
signals:
    void bindablePropertyChanged();
};
