/**
 * @file kalman_filter.c
 * @brief 一维标量卡尔曼滤波器实现
 *
 * 对连续采样值做平滑滤波：每来一个采样点执行一次“预测 + 校正”迭代，
 * 输出滤波后的估计值。常用于平滑 CPU 占用率等抖动数据，避免 UI 数值跳变。
 * 算法与 MCU 裸机上的写法一致，无操作系统依赖。
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "kalman_filter.h"

void KalmanInit(KalmanFilter *kf, double initial_state, double initial_error,
                double process_noise, double measurement_noise)
{
    kf->v = initial_state;
    kf->P = initial_error;
    kf->Q = process_noise;
    kf->R = measurement_noise;
}

void KalmanUpdate(KalmanFilter *kf, double measurement, int first)
{
    /* 第一个采样点没有历史可预测，直接用它作为初始状态就地初始化，
     * Q/R 为经验调参值：R 小（0.03）表示较信任测量值、响应快，Q=0.4 允许状态适度波动 */
    if (first)
        KalmanInit(kf, measurement, 1, 0.4, 0.03);

    /* 预测步：状态先按“会随过程噪声漂移”外推，同时放大估计误差 P */
    kf->v += kf->P * kf->Q;
    kf->P += kf->P * kf->Q + kf->Q * kf->P;

    /* 校正步：按 P 与 R 的比例算出卡尔曼增益 K，
     * K 越大越信测量值（跟踪快），越小越信预测值（更平滑） */
    kf->K = kf->P / (kf->P + kf->R);
    kf->v = kf->v + kf->K * (measurement - kf->v);
    kf->P = (1 - kf->K) * kf->P;
}

