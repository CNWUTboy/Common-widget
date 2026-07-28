#pragma once

/**
 * @file ISControl.h
 * @brief 控件能力接口 ISControl 定义。
 *
 * 供主题/语言等管理器统一操作各控件的抽象接口。
 */

#include <QWidget>

/**
 * @brief 供管理器统一操作各控件的能力接口。
 *
 * 各控件实现本接口后可被 ThemeManager、LanguageManager 等管理器注册与统一调度，
 * 用于套用主题与语言切换重译。
 */
class ISControl {
public:
    virtual ~ISControl() = default;
    /**
     * @brief 取得自身 QWidget，用于套主题。
     * @return 控件对应的 QWidget 指针。
     */
    virtual QWidget* asWidget() = 0;
    /**
     * @brief 语言切换时重译文案。
     */
    virtual void retranslate() = 0;
};
