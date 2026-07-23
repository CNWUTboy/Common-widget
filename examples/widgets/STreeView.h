#pragma once
#include <QTreeView>
#include "slabel/SControl.h"
class STreeView : public SControl<QTreeView> {
    Q_OBJECT
public:
    using SControl<QTreeView>::SControl;
};
