#pragma once
#include <QDialog>
#include <QMessageBox>
#include <QEvent>
#include "slabel/SControl.h"
class SLABEL_EXPORT SDialog : public SControl<QDialog> {
    Q_OBJECT
public:
    using SControl<QDialog>::SControl;
protected:
    void changeEvent(QEvent* e) override {
        if (e->type() == QEvent::LanguageChange) retranslate();
        QDialog::changeEvent(e);
    }
};
