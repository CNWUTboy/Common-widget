#include "slabel/SControlBridge.h"
#include "slabel/ThemeManager.h"
#include "slabel/LanguageManager.h"
#include <QMetaObject>
#include <QCoreApplication>
#include <QVariant>

SControlBridge::SControlBridge(QWidget* target, QObject* parent)
    : QObject(parent), m_target(target), m_engine(target) {
    ThemeManager::instance().registerControl(this);
    LanguageManager::instance().registerControl(this);
    m_target->installEventFilter(this);
}

SControlBridge::~SControlBridge() {
    ThemeManager::instance().unregisterControl(this);
    LanguageManager::instance().unregisterControl(this);
}

void SControlBridge::setTextTr(const char* sourceText) {
    m_sourceText = QByteArray(sourceText);
    retranslate();
}

void SControlBridge::retranslate() {
    if (m_sourceText.isEmpty()) return;
    // 运行时属性查找：目标类型不透明，靠 QMetaObject 判断是否有 "text" 属性
    // （QLabel/QPushButton/QLineEdit 等有文本控件都有），没有则安全跳过。
    if (m_target->metaObject()->indexOfProperty("text") < 0) return;
    m_target->setProperty("text", QCoreApplication::translate("slabel", m_sourceText.constData()));
}

bool SControlBridge::eventFilter(QObject* obj, QEvent* e) {
    if (obj == m_target && e->type() == QEvent::LanguageChange) retranslate();
    return QObject::eventFilter(obj, e);
}

SControlBridge* slabelAttach(QWidget* widget) {
    return new SControlBridge(widget, widget);
}
