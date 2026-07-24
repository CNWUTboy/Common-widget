#pragma once
#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QHash>
#include <QPointer>
#include <QString>
#include <functional>
#include "slabel/SGlobal.h"

// 单条绑定：连接一端 NOTIFY 信号，把值写到另一端，带防循环标志。
// 两端用 QPointer 观察生命周期：任一端销毁时自动从 registry 摘除并自毁，
// 避免"对端已析构、自身仍持裸指针"导致的 use-after-free。
// 注：Binding 为内部实现类（消费者不直接命名，均通过 BindingEngine 使用），
// 弃用标记只加在对外入口 BindingEngine / SBindableObject 上，避免头文件自引用噪声。
class SLABEL_EXPORT Binding : public QObject {
    Q_OBJECT
public:
    Binding(const QString& registryKey, QObject* a, const QByteArray& propA,
            QObject* b, const QByteArray& propB, bool twoWay);
    void observe(std::function<void(const QVariant&)> cb);
    // 是否正处于自身槽函数（syncAToB/syncBToA/fireCallback）调用栈中；
    // 供 BindingEngine::unbind 判断能否立即 delete。
    bool isDispatching() const { return m_dispatching; }
public slots:
    void syncAToB();
    void syncBToA();
    void fireCallback();
private slots:
    void onEndpointDestroyed();
private:
    static void connectNotify(QObject* obj, const QByteArray& prop,
                              Binding* self, const char* slot);
    QString m_key;
    QPointer<QObject> m_a; QByteArray m_propA;
    QPointer<QObject> m_b; QByteArray m_propB;
    bool m_updating = false;
    bool m_dispatching = false;
    std::function<void(const QVariant&)> m_cb;
};

// 限制：registry 以 (对象地址, 属性名) 为 key，每个端点+属性组合只维护一条
// Binding；对同一 (对象, 属性) 重复调用 bind/observe 会自动 unbind 旧绑定
// （见实现），因此无法对同一属性同时叠加"双向绑定"与"额外 observe 监听"。
class SLABEL_EXPORT SLABEL_DEPRECATED(
    "状态绑定能力已弃用，将在后续版本移除；请改用原生 Qt 信号/槽或属性绑定") BindingEngine {
public:
    static void bind(QObject* a, const QByteArray& propA,
                     QObject* b, const QByteArray& propB);
    static void observe(QObject* a, const QByteArray& propA,
                        std::function<void(const QVariant&)> cb);
    static void unbind(QObject* a, const QByteArray& propA);
private:
    friend class Binding;
    static QHash<QString, Binding*>& registry();
    static QString key(QObject* a, const QByteArray& propA);
    // 仅当 key 当前仍指向 expected 时才摘除（防止误删已被 rebind 替换的新绑定）
    static void removeFromRegistry(const QString& key, Binding* expected);
};
