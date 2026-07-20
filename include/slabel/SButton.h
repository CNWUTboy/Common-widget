#pragma once
#include <QPushButton>
#include "slabel/SGlobal.h"
#include "slabel/SControl.h"

class SLABEL_EXPORT SButton : public SControl<QPushButton> {
    Q_OBJECT
public:
    using SControl<QPushButton>::SControl;
};
