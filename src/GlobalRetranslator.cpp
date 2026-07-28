/**
 * @file GlobalRetranslator.cpp
 * @brief GlobalRetranslator 的实现：控件/动作文本捕获、整树回填、model/view 刷新与源串正反查解析。
 */
#include "slabel/GlobalRetranslator.h"

#include <QApplication>
#include <QWidget>
#include <QAction>
#include <QActionEvent>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QEvent>
#include <QFileInfo>
#include <QMetaObject>

GlobalRetranslator::GlobalRetranslator(QObject* parent) : QObject(parent) {}

void GlobalRetranslator::addLanguage(const QString& name, const QString& qmPath,
                                     const QString& tsPath) {
    m_qmPaths.insert(name, qmPath);
    if (!tsPath.isEmpty() && QFileInfo::exists(tsPath))
        m_catalog.addLanguageFile(name, tsPath);
}

void GlobalRetranslator::installOn(QApplication& app) {
    app.installEventFilter(this);
}

bool GlobalRetranslator::eventFilter(QObject* obj, QEvent* e) {
    if (m_translating) return false;              // 回填期间不自捕获
    switch (e->type()) {
    case QEvent::Polish:
    case QEvent::Show:
        if (auto* w = qobject_cast<QWidget*>(obj)) captureWidget(w);
        break;
    case QEvent::ActionAdded:
        if (auto* ae = static_cast<QActionEvent*>(e)) captureAction(ae->action());
        break;
    default:
        break;
    }
    return false;                                 // 绝不吞事件
}

bool GlobalRetranslator::hasSlotId(const WidgetEntry& e, const QByteArray& id) const {
    for (const CapturedSlot& s : e.caps)
        if (s.slot.id == id) return true;
    return false;
}

void GlobalRetranslator::captureWidget(QWidget* w) {
    WidgetEntry& entry = m_registry[w];
    if (entry.w.isNull()) {
        entry.w = w;
        connect(w, &QObject::destroyed, this, [this](QObject* o) {
            m_registry.remove(o);
            m_actions.remove(o);
            m_actionLang.remove(o);
        });
    }
    // model/view 单独登记，供切换时刷新
    if (auto* view = qobject_cast<QAbstractItemView*>(w)) {
        bool tracked = false;
        for (const auto& v : m_views) if (v == view) { tracked = true; break; }
        if (!tracked) m_views.push_back(view);
    }
    // 该控件上已存在的 QAction（如 menubar/toolbar 在 Polish 前 addAction）
    for (QAction* a : w->actions()) captureAction(a);

    for (TextSlot& s : TextAccessorRegistry::instance().slotsFor(w)) {
        if (hasSlotId(entry, s.id)) continue;     // 去重（含运行时新增的 item/tab）
        const QString cur = s.get(w);
        if (cur.isEmpty()) continue;
        entry.caps.push_back({ s, cur, m_current });
        if (m_current != m_sourceLang)            // 切换后才出现的控件：立即本地化该槽
            s.set(w, resolve(cur, m_current, w->metaObject()));
    }
}

void GlobalRetranslator::captureAction(QAction* a) {
    if (!a || m_actions.contains(a)) return;
    const QString cur = a->text();
    if (cur.isEmpty()) return;
    m_actions.insert(a, cur);
    m_actionLang.insert(a, m_current);
    connect(a, &QObject::destroyed, this, [this](QObject* o) {
        m_actions.remove(o);
        m_actionLang.remove(o);
    });
    if (m_current != m_sourceLang)
        a->setText(resolve(cur, m_current, a->metaObject()));
}

bool GlobalRetranslator::setLanguage(const QString& name) {
    if (name == m_sourceLang) {
        QApplication::removeTranslator(&m_translator);
        m_current = name;
        m_translating = true;
        retranslateAll();
        m_translating = false;
        return true;
    }
    auto it = m_qmPaths.constFind(name);
    if (it == m_qmPaths.constEnd())
        return false;
    // probe-load：失败则不动现状（参照 LanguageManager 的保护写法）
    QTranslator probe;
    if (!probe.load(it.value()))
        return false;
    QApplication::removeTranslator(&m_translator);
    m_translator.load(it.value());
    QApplication::installTranslator(&m_translator);
    m_current = name;
    m_translating = true;
    retranslateAll();
    m_translating = false;
    return true;
}

void GlobalRetranslator::retranslateAll() {
    for (auto it = m_registry.begin(); it != m_registry.end(); ) {
        WidgetEntry& e = it.value();
        if (e.w.isNull()) { it = m_registry.erase(it); continue; }
        QWidget* w = e.w.data();
        const QMetaObject* mo = w->metaObject();
        for (CapturedSlot& s : e.caps)
            s.slot.set(w, resolve(s.captured, s.capLang, mo));
        ++it;
    }
    for (auto it = m_actions.begin(); it != m_actions.end(); ++it) {
        auto* a = qobject_cast<QAction*>(it.key());
        if (!a) continue;
        a->setText(resolve(it.value(), m_actionLang.value(it.key(), m_sourceLang), a->metaObject()));
    }
    refreshViews();
}

void GlobalRetranslator::refreshViews() {
    for (const auto& v : m_views) {
        if (v.isNull()) continue;
        QAbstractItemView* view = v.data();
        view->doItemsLayout();                    // 强制重新向 model 取数 -> data() 内 tr() 重算
        if (view->viewport()) view->viewport()->update();
        for (QHeaderView* h : view->findChildren<QHeaderView*>())
            if (h->viewport()) h->viewport()->update();
    }
}

QString GlobalRetranslator::resolve(const QString& captured, const QString& capLang,
                                    const QMetaObject* mo) const {
    // 先还原出源串
    QString source = (capLang == m_sourceLang) ? captured : toSource(captured, capLang);
    if (source.isEmpty())
        return captured;                          // 无法还原源串，原样保留
    if (m_current == m_sourceLang)
        return source;                            // 切回源语言：直接显示源串
    const QString t = forward(source, mo);
    return t.isEmpty() ? source : t;
}

QString GlobalRetranslator::forward(const QString& source, const QMetaObject* mo) const {
    // 主路径：沿控件类继承链逐级作为 candidate context
    for (const QMetaObject* m = mo; m; m = m->superClass()) {
        const QString t = m_translator.translate(m->className(), source.toUtf8().constData());
        if (!t.isEmpty()) return t;
    }
    // 兜底：源串反查目录（覆盖"文字由外层类 tr() 设置"，context 不在控件类链上）
    const QStringList cands = m_catalog.translationsFor(m_current, source);
    if (cands.size() == 1) return cands.first();  // 歧义(>1)或无(0)则不猜
    return QString();
}

QString GlobalRetranslator::toSource(const QString& captured, const QString& capLang) const {
    const QStringList s = m_catalog.sourcesFor(capLang, captured);
    if (s.size() == 1) return s.first();
    return QString();
}

void GlobalRetranslator::refreshSource(QWidget* w) {
    if (!w) return;
    m_registry.remove(w);
    captureWidget(w);
}
