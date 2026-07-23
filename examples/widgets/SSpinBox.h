#pragma once
#include <QSpinBox>
#include "slabel/SControl.h"
class SSpinBox : public SControl<QSpinBox> {
    Q_OBJECT
public:
    using SControl<QSpinBox>::SControl;
};
