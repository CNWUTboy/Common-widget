#pragma once
#include <QWidget>
#include <QPainter>
#include "slabel/SControl.h"
#include "slabel/ThemeManager.h"

// 自绘控件示例：用主题 token 上色，主题切换时自动重绘。
// 证明自定义自绘控件也能接入 slabel 主题体系。
class SSwatch : public SControl<QWidget> {
    Q_OBJECT
public:
    explicit SSwatch(QWidget* parent = nullptr) : SControl<QWidget>(parent) {
        setMinimumHeight(24);
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
                this, [this]{ update(); }); // 切主题即重绘
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        QColor c = ThemeManager::instance().token("primary");
        if (!c.isValid()) c = Qt::gray;
        p.fillRect(rect(), c);
    }
};
