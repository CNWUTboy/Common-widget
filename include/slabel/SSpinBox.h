#pragma once
#include <QSpinBox>
#include "slabel/SControl.h"
class SLABEL_EXPORT SSpinBox : public SControl<QSpinBox> {
    Q_OBJECT
public:
    using SControl<QSpinBox>::SControl;
};
