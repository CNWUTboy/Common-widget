#pragma once

// 控件操作状态：具备"触发操作"语义的控件（如按钮）在一次操作生命周期内所处的阶段
enum class OperationState {
    Idle,     // 待命
    Busy,     // 执行中
    Success,  // 成功
    Failure   // 失败
};
