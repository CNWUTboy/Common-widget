#pragma once
#include <QRadioButton>
#include "slabel/SControl.h"
class SRadioButton : public SControl<QRadioButton> {
    Q_OBJECT
public:
    using SControl<QRadioButton>::SControl;
};
