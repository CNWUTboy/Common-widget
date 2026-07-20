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

Binding::Binding(QObject* a, const QByteArray& propA,
                 QObject* b, const QByteArray& propB, bool twoWay)
    : m_a(a), m_propA(propA), m_b(b), m_propB(propB) {
    connectNotify(a, propA, this, SLOT(syncAToB()));
    if (twoWay)
        connectNotify(b, propB, this, SLOT(syncBToA()));
    syncAToB(); // 初始同步 a→b
}

void Binding::observe(std::function<void(const QVariant&)> cb) {
    m_cb = std::move(cb);
    connectNotify(m_a, m_propA, this, SLOT(fireCallback()));
}

void Binding::syncAToB() {
    if (m_updating) return;
    m_updating = true;
    m_b->setProperty(m_propB.constData(), m_a->property(m_propA.constData()));
    m_updating = false;
}

void Binding::syncBToA() {
    if (m_updating) return;
    m_updating = true;
    m_a->setProperty(m_propA.constData(), m_b->property(m_propB.constData()));
    m_updating = false;
}

void Binding::fireCallback() {
    if (m_cb)
        m_cb(m_a->property(m_propA.constData()));
}

QHash<QString, Binding*>& BindingEngine::registry() {
    static QHash<QString, Binding*> r;
    return r;
}

QString BindingEngine::key(QObject* a, const QByteArray& propA) {
    return QString::number(reinterpret_cast<quintptr>(a)) + "/" + QString::fromUtf8(propA);
}

void BindingEngine::bind(QObject* a, const QByteArray& propA,
                         QObject* b, const QByteArray& propB) {
    auto* binding = new Binding(a, propA, b, propB, /*twoWay=*/true);
    registry().insert(key(a, propA), binding);
}

void BindingEngine::observe(QObject* a, const QByteArray& propA,
                            std::function<void(const QVariant&)> cb) {
    auto* binding = new Binding(a, propA, a, propA, /*twoWay=*/false);
    binding->observe(std::move(cb));
    registry().insert(key(a, propA), binding);
}

void BindingEngine::unbind(QObject* a, const QByteArray& propA) {
    const QString k = key(a, propA);
    if (auto* b = registry().take(k))
        delete b; // 立即断开信号连接；deleteLater 需要事件循环驱动才会真正断开，会导致 unbind 后仍短暂同步
}
