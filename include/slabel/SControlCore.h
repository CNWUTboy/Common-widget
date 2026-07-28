#pragma once
/**
 * @file SControlCore.h
 * @brief 信号载体 SControlCore：为非 QObject 的 CRTP 模板/组合类提供 Qt 信号能力。
 */
#include <QObject>
#include "slabel/SGlobal.h"

/**
 * @brief 为非 QObject 的 CRTP 模板提供 signal 的小助手。
 *
 * 作为成员被组合进不便直接继承 QObject 的类型，转发操作状态变更信号。
 */
class SLABEL_EXPORT SControlCore : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造信号载体。
     * @param parent 父对象，交由 Qt 父子链管理生命周期。
     */
    explicit SControlCore(QObject* parent = nullptr) : QObject(parent) {}
    /**
     * @brief 发射 operationStateChanged 信号，供持有者在操作状态变化时调用。
     */
    void notifyOperationStateChanged() { emit operationStateChanged(); }
signals:
    /**
     * @brief 当操作状态发生变化时发射。
     */
    void operationStateChanged();
};
