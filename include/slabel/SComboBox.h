#pragma once
#include <QComboBox>
#include "slabel/SControl.h"
class SLABEL_EXPORT SComboBox : public SControl<QComboBox> {
    Q_OBJECT
public:
    using SControl<QComboBox>::SControl;
};
