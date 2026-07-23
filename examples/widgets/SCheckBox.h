#pragma once
#include <QCheckBox>
#include "slabel/SControl.h"
class SCheckBox : public SControl<QCheckBox> {
    Q_OBJECT
public:
    using SControl<QCheckBox>::SControl;
};
