#pragma once
#include <QListView>
#include "slabel/SControl.h"
class SLABEL_EXPORT SListView : public SControl<QListView> {
    Q_OBJECT
public:
    using SControl<QListView>::SControl;
};
