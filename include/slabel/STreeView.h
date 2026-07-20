#pragma once
#include <QTreeView>
#include "slabel/SControl.h"
class SLABEL_EXPORT STreeView : public SControl<QTreeView> {
    Q_OBJECT
public:
    using SControl<QTreeView>::SControl;
};
