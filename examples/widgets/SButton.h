#pragma once
#include <QPushButton>
#include <type_traits>
#include <utility>
#include "slabel/SControl.h"

class SButton : public SControl<QPushButton> {
    Q_OBJECT
public:
    // 转发构造 + 接入操作状态反馈能力：把 clicked() 接到 triggerOperation()。
    // SFINAE 排除单参数且为 SButton 派生/同类的情况，避免劫持拷贝/移动构造。
    template<typename... Args,
             typename = std::enable_if_t<
                 !(sizeof...(Args) == 1 &&
                   (std::is_base_of_v<SButton, std::decay_t<Args>> || ...))>>
    explicit SButton(Args&&... args)
        : SControl<QPushButton>(std::forward<Args>(args)...) {
        connect(this, &QPushButton::clicked, this, &SButton::triggerOperation);
    }
};
