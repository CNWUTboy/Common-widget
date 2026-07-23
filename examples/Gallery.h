#pragma once
#include <QWidget>
#include "widgets/SLineEdit.h"
#include "widgets/SLabel.h"

class Gallery : public QWidget {
    Q_OBJECT
public:
    explicit Gallery(QWidget* parent = nullptr);
private:
    bool m_dark = false;
    bool m_en = false;
    SLineEdit* m_edit = nullptr;
    SLabel* m_mirror = nullptr;
};
