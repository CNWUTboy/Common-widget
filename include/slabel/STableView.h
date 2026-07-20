#pragma once
#include <QTableView>
#include "slabel/SControl.h"
class SLABEL_EXPORT STableView : public SControl<QTableView> {
    Q_OBJECT
public:
    using SControl<QTableView>::SControl;
};
