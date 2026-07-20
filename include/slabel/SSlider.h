#pragma once
#include <QSlider>
#include "slabel/SControl.h"
class SLABEL_EXPORT SSlider : public SControl<QSlider> {
    Q_OBJECT
public:
    using SControl<QSlider>::SControl;
};
