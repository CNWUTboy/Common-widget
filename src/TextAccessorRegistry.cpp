/**
 * @file TextAccessorRegistry.cpp
 * @brief TextAccessorRegistry 的实现：内置各 Qt 控件类型的文本槽枚举器与按类型收集去重逻辑。
 */
#include "slabel/TextAccessorRegistry.h"

#include <QWidget>
#include <QMetaObject>
#include <QAbstractButton>
#include <QLabel>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTabBar>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QMenu>
#include <QTextDocument> // Qt::mightBeRichText

namespace {

// 便捷：构造一个单属性文本槽
template <typename W, typename Getter, typename Setter>
TextSlot propSlot(const QByteArray& id, Getter g, Setter s) {
    TextSlot slot;
    slot.id = id;
    slot.get = [g](QWidget* w) -> QString { return (static_cast<W*>(w)->*g)(); };
    slot.set = [s](QWidget* w, const QString& t) { (static_cast<W*>(w)->*s)(t); };
    return slot;
}

// 便捷：注册一个"仅一个（或按条件产出）槽"的类型访问器
TypeAccessor typeAcc(const QMetaObject* mo,
                     std::function<QVector<TextSlot>(QWidget*)> enumerate) {
    TypeAccessor a;
    a.type = mo;
    a.enumerate = std::move(enumerate);
    return a;
}

} // namespace

TextAccessorRegistry& TextAccessorRegistry::instance() {
    static TextAccessorRegistry s;
    if (!s.m_builtinDone) s.registerBuiltinTypes();
    return s;
}

void TextAccessorRegistry::registerType(TypeAccessor accessor) {
    if (accessor.type && accessor.enumerate)
        m_accessors.push_back(std::move(accessor));
}

QVector<TextSlot> TextAccessorRegistry::slotsFor(QWidget* w) const {
    QVector<TextSlot> out;
    if (!w) return out;
    const QMetaObject* mo = w->metaObject();
    for (const TypeAccessor& a : m_accessors) {
        if (!mo->inherits(a.type)) continue;
        for (TextSlot& s : a.enumerate(w)) {
            bool dup = false;
            for (const TextSlot& e : out)
                if (e.id == s.id) { dup = true; break; }
            if (!dup) out.push_back(std::move(s));
        }
    }
    return out;
}

void TextAccessorRegistry::registerBuiltinTypes() {
    if (m_builtinDone) return;
    m_builtinDone = true;

    // 窗口标题（任何 QWidget；仅非空时产出，避免为每个控件都登记空槽）
    registerType(typeAcc(&QWidget::staticMetaObject, [](QWidget* w) {
        QVector<TextSlot> v;
        if (!w->windowTitle().isEmpty())
            v.push_back(propSlot<QWidget>("windowTitle",
                &QWidget::windowTitle, &QWidget::setWindowTitle));
        return v;
    }));

    // 按钮文字（QPushButton/QCheckBox/QRadioButton/QToolButton 等）
    registerType(typeAcc(&QAbstractButton::staticMetaObject, [](QWidget*) {
        QVector<TextSlot> v;
        v.push_back(propSlot<QAbstractButton>("text",
            &QAbstractButton::text, &QAbstractButton::setText));
        return v;
    }));

    // 标签文字（仅纯文本；富文本跳过以免破坏 HTML 标记）
    registerType(typeAcc(&QLabel::staticMetaObject, [](QWidget* w) {
        QVector<TextSlot> v;
        auto* l = static_cast<QLabel*>(w);
        if (!Qt::mightBeRichText(l->text()))
            v.push_back(propSlot<QLabel>("text", &QLabel::text, &QLabel::setText));
        return v;
    }));

    // 分组框标题
    registerType(typeAcc(&QGroupBox::staticMetaObject, [](QWidget*) {
        QVector<TextSlot> v;
        v.push_back(propSlot<QGroupBox>("title",
            &QGroupBox::title, &QGroupBox::setTitle));
        return v;
    }));

    // 输入框占位符
    registerType(typeAcc(&QLineEdit::staticMetaObject, [](QWidget*) {
        QVector<TextSlot> v;
        v.push_back(propSlot<QLineEdit>("placeholder",
            &QLineEdit::placeholderText, &QLineEdit::setPlaceholderText));
        return v;
    }));

    // 菜单标题
    registerType(typeAcc(&QMenu::staticMetaObject, [](QWidget*) {
        QVector<TextSlot> v;
        v.push_back(propSlot<QMenu>("title", &QMenu::title, &QMenu::setTitle));
        return v;
    }));

    // 下拉框各 item（运行时按项数枚举）
    registerType(typeAcc(&QComboBox::staticMetaObject, [](QWidget* w) {
        QVector<TextSlot> v;
        auto* cb = static_cast<QComboBox*>(w);
        for (int i = 0; i < cb->count(); ++i) {
            TextSlot s;
            s.id = "item#" + QByteArray::number(i);
            s.get = [i](QWidget* w) { return static_cast<QComboBox*>(w)->itemText(i); };
            s.set = [i](QWidget* w, const QString& t) { static_cast<QComboBox*>(w)->setItemText(i, t); };
            v.push_back(std::move(s));
        }
        return v;
    }));

    // 选项卡各 tab（QTabBar；QTabWidget 内部含 QTabBar，会被一并捕获，故只登记 QTabBar 避免重复）
    registerType(typeAcc(&QTabBar::staticMetaObject, [](QWidget* w) {
        QVector<TextSlot> v;
        auto* tb = static_cast<QTabBar*>(w);
        for (int i = 0; i < tb->count(); ++i) {
            TextSlot s;
            s.id = "tab#" + QByteArray::number(i);
            s.get = [i](QWidget* w) { return static_cast<QTabBar*>(w)->tabText(i); };
            s.set = [i](QWidget* w, const QString& t) { static_cast<QTabBar*>(w)->setTabText(i, t); };
            v.push_back(std::move(s));
        }
        return v;
    }));

    // QListWidget 各项
    registerType(typeAcc(&QListWidget::staticMetaObject, [](QWidget* w) {
        QVector<TextSlot> v;
        auto* lw = static_cast<QListWidget*>(w);
        for (int i = 0; i < lw->count(); ++i) {
            TextSlot s;
            s.id = "litem#" + QByteArray::number(i);
            s.get = [i](QWidget* w) {
                auto* it = static_cast<QListWidget*>(w)->item(i);
                return it ? it->text() : QString();
            };
            s.set = [i](QWidget* w, const QString& t) {
                if (auto* it = static_cast<QListWidget*>(w)->item(i)) it->setText(t);
            };
            v.push_back(std::move(s));
        }
        return v;
    }));

    // QTreeWidget 顶层项 + 表头（列 0..columnCount-1，仅顶层，v1 不递归子项）
    registerType(typeAcc(&QTreeWidget::staticMetaObject, [](QWidget* w) {
        QVector<TextSlot> v;
        auto* tw = static_cast<QTreeWidget*>(w);
        const int cols = tw->columnCount();
        for (int c = 0; c < cols; ++c) {
            TextSlot h;
            h.id = "hheader#" + QByteArray::number(c);
            h.get = [c](QWidget* w) {
                auto* it = static_cast<QTreeWidget*>(w)->headerItem();
                return it ? it->text(c) : QString();
            };
            h.set = [c](QWidget* w, const QString& t) {
                if (auto* it = static_cast<QTreeWidget*>(w)->headerItem()) it->setText(c, t);
            };
            v.push_back(std::move(h));
        }
        for (int i = 0; i < tw->topLevelItemCount(); ++i)
            for (int c = 0; c < cols; ++c) {
                TextSlot s;
                s.id = "titem#" + QByteArray::number(i) + "." + QByteArray::number(c);
                s.get = [i, c](QWidget* w) {
                    auto* it = static_cast<QTreeWidget*>(w)->topLevelItem(i);
                    return it ? it->text(c) : QString();
                };
                s.set = [i, c](QWidget* w, const QString& t) {
                    if (auto* it = static_cast<QTreeWidget*>(w)->topLevelItem(i)) it->setText(c, t);
                };
                v.push_back(std::move(s));
            }
        return v;
    }));

    // QTableWidget 单元格 + 水平/垂直表头项
    registerType(typeAcc(&QTableWidget::staticMetaObject, [](QWidget* w) {
        QVector<TextSlot> v;
        auto* t = static_cast<QTableWidget*>(w);
        for (int c = 0; c < t->columnCount(); ++c) {
            TextSlot h;
            h.id = "hh#" + QByteArray::number(c);
            h.get = [c](QWidget* w) {
                auto* it = static_cast<QTableWidget*>(w)->horizontalHeaderItem(c);
                return it ? it->text() : QString();
            };
            h.set = [c](QWidget* w, const QString& t) {
                if (auto* it = static_cast<QTableWidget*>(w)->horizontalHeaderItem(c)) it->setText(t);
            };
            v.push_back(std::move(h));
        }
        for (int r = 0; r < t->rowCount(); ++r)
            for (int c = 0; c < t->columnCount(); ++c) {
                TextSlot s;
                s.id = "cell#" + QByteArray::number(r) + "." + QByteArray::number(c);
                s.get = [r, c](QWidget* w) {
                    auto* it = static_cast<QTableWidget*>(w)->item(r, c);
                    return it ? it->text() : QString();
                };
                s.set = [r, c](QWidget* w, const QString& t) {
                    if (auto* it = static_cast<QTableWidget*>(w)->item(r, c)) it->setText(t);
                };
                v.push_back(std::move(s));
            }
        return v;
    }));
}
