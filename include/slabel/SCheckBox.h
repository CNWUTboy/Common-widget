#pragma once
#include <QCheckBox>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SCheckBox : public SControl<QCheckBox> {
    Q_OBJECT
public:
    using SControl<QCheckBox>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QCheckBox::changeEvent(e);
    }
};
