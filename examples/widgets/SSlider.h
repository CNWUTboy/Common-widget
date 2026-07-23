#pragma once
#include <QSlider>
#include "slabel/SControl.h"
class SSlider : public SControl<QSlider> {
    Q_OBJECT
public:
    using SControl<QSlider>::SControl;
};
