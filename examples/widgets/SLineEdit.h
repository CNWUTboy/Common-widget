#pragma once
#include <QLineEdit>
#include "slabel/SControl.h"
class SLineEdit : public SControl<QLineEdit> {
    Q_OBJECT
public:
    using SControl<QLineEdit>::SControl;
};
