#pragma once
#include <QWidget>
#include <QPainter>
#include <QEvent>
#include "slabel/ThemeManager.h"

// 独立自定义控件示例：不继承 SControl/SControlBridge，纯 QWidget 实现。
// 用纯 Qt 原生方式接入主题与翻译，不依赖 slabel 的控件封装：
//   - 主题：订阅 ThemeManager::themeChanged，用 colorToken 取色，切主题重绘；
//   - 翻译：paintEvent 内文字用 tr() 标记，覆写 changeEvent 捕获
//           QEvent::LanguageChange 后 update() 重绘，tr() 自动取新译文。
class StatusLight : public QWidget {
    Q_OBJECT
public:
    enum class Status { Offline, Warning, Online };

    explicit StatusLight(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumSize(80, 20);
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
                this, [this]{ update(); }); // 切主题即重绘
    }

    void setStatus(Status s) {
        if (m_status == s) return;
        m_status = s;
        update();
    }
    Status status() const { return m_status; }

protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange)
            update(); // 切语言即重绘，paintEvent 内 tr() 自动取新译文
        QWidget::changeEvent(e);
    }

    // 主题取色：优先用当前主题的 token，主题未定义该 token 时回退到给定默认色
    static QColor themeColor(const char* token, const char* fallback) {
        QColor c = ThemeManager::instance().colorToken(token);
        return c.isValid() ? c : QColor(fallback);
    }

    void paintEvent(QPaintEvent*) override {
        // 状态主色也走主题 token（@statusOnline/@statusWarning/@statusOffline），
        // 随主题切换而变；主题未声明时回退到语义默认色。
        QColor dot;
        QString label;
        switch (m_status) {
        case Status::Online:  dot = themeColor("statusOnline",  "#28a745"); label = tr("在线"); break;
        case Status::Warning: dot = themeColor("statusWarning", "#ffc107"); label = tr("警告"); break;
        case Status::Offline: dot = themeColor("statusOffline", "#6c757d"); label = tr("离线"); break;
        }
        // 描边、文字色同样走 token，随主题切换
        QColor ring = themeColor("border", "#a9a9a9");
        QColor textColor = themeColor("text", "#000000");

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int d = 12;
        QRectF dotRect(1.0, (height() - d) / 2.0, d, d);
        p.setPen(QPen(ring, 1));
        p.setBrush(dot);
        p.drawEllipse(dotRect);

        p.setPen(textColor);
        QRect textRect(int(dotRect.right()) + 6, 0,
                       width() - int(dotRect.right()) - 6, height());
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, label);
    }

private:
    Status m_status = Status::Offline;
};
