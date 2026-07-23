#pragma once
#include <QListView>
#include "slabel/SControl.h"
class SListView : public SControl<QListView> {
    Q_OBJECT
public:
    using SControl<QListView>::SControl;
};
