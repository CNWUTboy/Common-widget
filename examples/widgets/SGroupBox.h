#pragma once
#include <QGroupBox>
#include "slabel/SControl.h"
class SGroupBox : public SControl<QGroupBox> {
    Q_OBJECT
public:
    using SControl<QGroupBox>::SControl;
    // QGroupBox 没有 setText()，只有 setTitle()——重写 retranslate
    void retranslate() override {
        if (!m_sourceText.isEmpty())
            this->setTitle(QCoreApplication::translate("slabel", m_sourceText.constData()));
    }
};
