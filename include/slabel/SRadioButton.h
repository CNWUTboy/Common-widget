#pragma once
#include <QRadioButton>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SRadioButton : public SControl<QRadioButton> {
    Q_OBJECT
public:
    using SControl<QRadioButton>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QRadioButton::changeEvent(e);
    }
};
