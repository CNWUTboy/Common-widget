#pragma once
#include <QDialog>
#include <QMessageBox>
#include "slabel/SControl.h"
class SDialog : public SControl<QDialog> {
    Q_OBJECT
public:
    using SControl<QDialog>::SControl;
};
