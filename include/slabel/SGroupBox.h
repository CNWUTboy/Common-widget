#pragma once
#include <QGroupBox>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SGroupBox : public SControl<QGroupBox> {
    Q_OBJECT
public:
    using SControl<QGroupBox>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QGroupBox::changeEvent(e);
    }
};
