#pragma once
#include <QTabWidget>
#include "slabel/SControl.h"
class SLABEL_EXPORT STabWidget : public SControl<QTabWidget> {
    Q_OBJECT
public:
    using SControl<QTabWidget>::SControl;
};
