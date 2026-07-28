#pragma once
/**
 * @file BindingEngine.h
 * @brief 属性绑定引擎：在两个 QObject 的属性之间建立单向/双向同步与变更监听。
 *
 * 提供内部实现类 Binding（单条绑定）与对外静态入口 BindingEngine
 * （以 registry 管理全部绑定）。该能力已弃用，建议改用原生 Qt 信号/槽或属性绑定。
 */
#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QHash>
#include <QPointer>
#include <QString>
#include <functional>
#include "slabel/SGlobal.h"

/**
 * @brief 单条属性绑定：连接一端 NOTIFY 信号，把值写到另一端，带防循环标志。
 *
 * 两端用 QPointer 观察生命周期：任一端销毁时自动从 registry 摘除并自毁，
 * 避免“对端已析构、自身仍持裸指针”导致的 use-after-free。
 * 注：Binding 为内部实现类（消费者不直接命名，均通过 BindingEngine 使用），
 * 弃用标记只加在对外入口 BindingEngine / SBindableObject 上，避免头文件自引用噪声。
 */
class SLABEL_EXPORT Binding : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造一条绑定并完成初始同步（a→b）。
     * @param registryKey 该绑定在 registry 中的键。
     * @param a 端点 A 对象。
     * @param propA 端点 A 上参与绑定的属性名。
     * @param b 端点 B 对象。
     * @param propB 端点 B 上参与绑定的属性名。
     * @param twoWay 为 true 时同时监听 B 的变更并回写到 A（双向）。
     */
    Binding(const QString& registryKey, QObject* a, const QByteArray& propA,
            QObject* b, const QByteArray& propB, bool twoWay);
    /**
     * @brief 追加变更监听：A 的属性变化时以新值调用回调。
     * @param cb 变更回调，参数为 A 属性的当前值。
     */
    void observe(std::function<void(const QVariant&)> cb);
    /**
     * @brief 是否正处于自身槽函数（syncAToB/syncBToA/fireCallback）调用栈中。
     * @return 处于派发中返回 true；供 BindingEngine::unbind 判断能否立即 delete。
     */
    bool isDispatching() const { return m_dispatching; }
public slots:
    /**
     * @brief 将端点 A 的属性值同步到端点 B（带防循环保护）。
     */
    void syncAToB();
    /**
     * @brief 将端点 B 的属性值同步到端点 A（带防循环保护）。
     */
    void syncBToA();
    /**
     * @brief 触发 observe 注册的变更回调，传入 A 属性的当前值。
     */
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

/**
 * @brief 属性绑定的对外静态入口，以全局 registry 管理所有绑定。
 *
 * 限制：registry 以 (对象地址, 属性名) 为 key，每个端点+属性组合只维护一条
 * Binding；对同一 (对象, 属性) 重复调用 bind/observe 会自动 unbind 旧绑定
 * （见实现），因此无法对同一属性同时叠加“双向绑定”与“额外 observe 监听”。
 *
 * @deprecated 状态绑定能力已弃用，将在后续版本移除；请改用原生 Qt 信号/槽或属性绑定。
 */
class SLABEL_EXPORT SLABEL_DEPRECATED(
    "状态绑定能力已弃用，将在后续版本移除；请改用原生 Qt 信号/槽或属性绑定") BindingEngine {
public:
    /**
     * @brief 在两个对象的属性间建立双向绑定，并立即以 A 的值同步一次。
     * @param a 端点 A 对象。
     * @param propA 端点 A 上参与绑定的属性名。
     * @param b 端点 B 对象。
     * @param propB 端点 B 上参与绑定的属性名。
     */
    static void bind(QObject* a, const QByteArray& propA,
                     QObject* b, const QByteArray& propB);
    /**
     * @brief 监听某对象属性的变更，值变化时调用回调（不做回写）。
     * @param a 被监听对象。
     * @param propA 被监听的属性名。
     * @param cb 变更回调，参数为该属性的当前值。
     */
    static void observe(QObject* a, const QByteArray& propA,
                        std::function<void(const QVariant&)> cb);
    /**
     * @brief 解除某对象属性上的绑定/监听并释放对应 Binding。
     * @param a 目标对象。
     * @param propA 目标属性名。
     */
    static void unbind(QObject* a, const QByteArray& propA);
private:
    friend class Binding;
    static QHash<QString, Binding*>& registry();
    static QString key(QObject* a, const QByteArray& propA);
    // 仅当 key 当前仍指向 expected 时才摘除（防止误删已被 rebind 替换的新绑定）
    static void removeFromRegistry(const QString& key, Binding* expected);
};
