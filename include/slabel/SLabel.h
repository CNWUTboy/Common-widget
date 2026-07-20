#pragma once
#include <QLabel>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SLabel : public SControl<QLabel> {
    Q_OBJECT
public:
    using SControl<QLabel>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QLabel::changeEvent(e);
    }
};
