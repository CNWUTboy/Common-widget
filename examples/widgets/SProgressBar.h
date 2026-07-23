#pragma once
#include <QProgressBar>
#include "slabel/SControl.h"
class SProgressBar : public SControl<QProgressBar> {
    Q_OBJECT
public:
    using SControl<QProgressBar>::SControl;
};
