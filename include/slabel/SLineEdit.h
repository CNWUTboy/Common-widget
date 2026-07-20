#pragma once
#include <QLineEdit>
#include "slabel/SControl.h"
class SLABEL_EXPORT SLineEdit : public SControl<QLineEdit> {
    Q_OBJECT
public:
    using SControl<QLineEdit>::SControl;
};
