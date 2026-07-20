#pragma once
#include <QWidget>

// 供管理器统一操作各控件的能力接口
class ISControl {
public:
    virtual ~ISControl() = default;
    virtual QWidget* asWidget() = 0;   // 取得自身 QWidget，用于套主题
    virtual void retranslate() = 0;    // 语言切换时重译文案
};
