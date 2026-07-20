#pragma once
#include <QPushButton>
#include <QEvent>
#include "slabel/SGlobal.h"
#include "slabel/SControl.h"

class SLABEL_EXPORT SButton : public SControl<QPushButton> {
    Q_OBJECT
public:
    using SControl<QPushButton>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange)
            retranslate();
        QPushButton::changeEvent(e);
    }
};
