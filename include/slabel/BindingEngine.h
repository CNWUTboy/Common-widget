#pragma once
#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QHash>
#include <functional>
#include "slabel/SGlobal.h"

// 单条绑定：连接一端 NOTIFY 信号，把值写到另一端，带防循环标志
class SLABEL_EXPORT Binding : public QObject {
    Q_OBJECT
public:
    Binding(QObject* a, const QByteArray& propA,
            QObject* b, const QByteArray& propB, bool twoWay);
    void observe(std::function<void(const QVariant&)> cb);
public slots:
    void syncAToB();
    void syncBToA();
    void fireCallback();
private:
    static void connectNotify(QObject* obj, const QByteArray& prop,
                              Binding* self, const char* slot);
    QObject* m_a; QByteArray m_propA;
    QObject* m_b; QByteArray m_propB;
    bool m_updating = false;
    std::function<void(const QVariant&)> m_cb;
};

class SLABEL_EXPORT BindingEngine {
public:
    static void bind(QObject* a, const QByteArray& propA,
                     QObject* b, const QByteArray& propB);
    static void observe(QObject* a, const QByteArray& propA,
                        std::function<void(const QVariant&)> cb);
    static void unbind(QObject* a, const QByteArray& propA);
private:
    static QHash<QString, Binding*>& registry();
    static QString key(QObject* a, const QByteArray& propA);
};
