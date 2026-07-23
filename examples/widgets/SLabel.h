#pragma once
#include <QLabel>
#include "slabel/SControl.h"
class SLabel : public SControl<QLabel> {
    Q_OBJECT
public:
    using SControl<QLabel>::SControl;
};
