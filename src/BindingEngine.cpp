#include "slabel/BindingEngine.h"
#include <QMetaProperty>
#include <QMetaMethod>

// 把 obj 的 prop 的 NOTIFY 信号连到 self 的具名 slot
void Binding::connectNotify(QObject* obj, const QByteArray& prop,
                            Binding* self, const char* slot) {
    const QMetaObject* mo = obj->metaObject();
    int pi = mo->indexOfProperty(prop.constData());
    if (pi < 0) return;
    QMetaProperty mp = mo->property(pi);
    if (!mp.hasNotifySignal()) return;
    QMetaMethod sig = mp.notifySignal();
    int si = self->metaObject()->indexOfSlot(
        QMetaObject::normalizedSignature(slot + 1).constData()); // slot 形如 "1syncAToB()"
    QMetaMethod sl = self->metaObject()->method(si);
    QObject::connect(obj, sig, self, sl);
}

Binding::Binding(const QString& registryKey, QObject* a, const QByteArray& propA,
                 QObject* b, const QByteArray& propB, bool twoWay)
    : m_key(registryKey), m_a(a), m_propA(propA), m_b(b), m_propB(propB) {
    connectNotify(a, propA, this, SLOT(syncAToB()));
    if (twoWay)
        connectNotify(b, propB, this, SLOT(syncBToA()));
    // 任一端销毁即自动摘除并自毁，避免对端悬垂后仍被 setProperty 访问（UAF）
    connect(a, &QObject::destroyed, this, &Binding::onEndpointDestroyed);
    if (b != a)
        connect(b, &QObject::destroyed, this, &Binding::onEndpointDestroyed);
    syncAToB(); // 初始同步 a→b
}

void Binding::observe(std::function<void(const QVariant&)> cb) {
    m_cb = std::move(cb);
    connectNotify(m_a, m_propA, this, SLOT(fireCallback()));
}

void Binding::syncAToB() {
    if (m_updating || !m_a || !m_b) return;
    m_updating = true;
    m_dispatching = true;
    m_b->setProperty(m_propB.constData(), m_a->property(m_propA.constData()));
    m_dispatching = false;
    m_updating = false;
}

void Binding::syncBToA() {
    if (m_updating || !m_a || !m_b) return;
    m_updating = true;
    m_dispatching = true;
    m_a->setProperty(m_propA.constData(), m_b->property(m_propB.constData()));
    m_dispatching = false;
    m_updating = false;
}

void Binding::fireCallback() {
    if (!m_cb || !m_a) return;
    m_dispatching = true;
    m_cb(m_a->property(m_propA.constData()));
    m_dispatching = false;
}

void Binding::onEndpointDestroyed() {
    // 只有当 registry 仍指向本对象时才摘除，避免误删已被 rebind 替换的新绑定；
    // 摘除后立即自毁——这里是在“另一个 QObject 的 destroyed 信号”发射期间删除
    // 本对象，而非本对象自身信号发射期间删除自身，属 Qt 支持的安全用法。
    BindingEngine::removeFromRegistry(m_key, this);
    delete this;
}

QHash<QString, Binding*>& BindingEngine::registry() {
    static QHash<QString, Binding*> r;
    return r;
}

QString BindingEngine::key(QObject* a, const QByteArray& propA) {
    return QString::number(reinterpret_cast<quintptr>(a)) + "/" + QString::fromUtf8(propA);
}

void BindingEngine::removeFromRegistry(const QString& k, Binding* expected) {
    auto it = registry().find(k);
    if (it != registry().end() && it.value() == expected)
        registry().erase(it);
}

void BindingEngine::bind(QObject* a, const QByteArray& propA,
                         QObject* b, const QByteArray& propB) {
    unbind(a, propA); // 先清理同 key 旧绑定，避免泄漏与重复同步
    const QString k = key(a, propA);
    auto* binding = new Binding(k, a, propA, b, propB, /*twoWay=*/true);
    registry().insert(k, binding);
}

void BindingEngine::observe(QObject* a, const QByteArray& propA,
                            std::function<void(const QVariant&)> cb) {
    unbind(a, propA); // 先清理同 key 旧绑定，避免泄漏与重复同步
    const QString k = key(a, propA);
    auto* binding = new Binding(k, a, propA, a, propA, /*twoWay=*/false);
    binding->observe(std::move(cb));
    registry().insert(k, binding);
}

void BindingEngine::unbind(QObject* a, const QByteArray& propA) {
    const QString k = key(a, propA);
    auto* b = registry().take(k);
    if (!b) return;
    if (b->isDispatching()) {
        // 从 Binding 自身回调（如 observe 的 cb）内部对同一 key 调用 unbind：
        // 此刻仍处于该 Binding 自身槽函数（信号发射）调用栈中，立即 delete
        // 会在发射尚未返回时销毁接收者本身，属 delete-during-emission，UAF
        // 风险；改为 deleteLater 延后到当前事件循环回合结束再析构。
        b->deleteLater();
    } else {
        delete b; // 立即断开信号连接；deleteLater 需要事件循环驱动才会真正断开，会导致 unbind 后仍短暂同步
    }
}
