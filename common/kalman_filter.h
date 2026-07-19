/**
 * @file kalman_filter.h
 * @brief 一维标量卡尔曼滤波器接口头文件
 *
 * 本模块用于对连续采样值（如 CPU 占用率、信号强度等跳变数据）做平滑滤波，
 * 滤掉采样噪声后再送给 UI 显示，避免界面数值/进度条抖动。
 * 算法与裸机上常用的卡尔曼滤波完全一致，只涉及浮点运算，无任何 Linux 依赖。
 */
#ifndef __KALMAN_FILTER_H__
#define __KALMAN_FILTER_H__

/**
 * @brief 卡尔曼滤波器状态结构体
 *
 * 每路需要滤波的数据各持有一个实例，调用方直接定义在栈上即可，
 * 使用前必须先调用 KalmanInit() 初始化（或由 KalmanUpdate() 的 first 参数代劳）。
 */
typedef struct
{
    double v;   /**< 当前状态估计值（滤波后的输出结果） */
    double P;   /**< 估计误差协方差，反映对当前估计值的不确定程度 */
    double Q;   /**< 过程噪声协方差：被测对象自身的真实波动强度，越大跟踪越快但越不平滑 */
    double R;   /**< 测量噪声协方差：采样值本身的噪声强度，越大越平滑但响应越迟钝 */
    double K;   /**< 卡尔曼增益（每次更新时内部计算，调用方无需关心） */
} KalmanFilter;

/**
 * @brief 初始化卡尔曼滤波器
 * @param kf                滤波器实例指针，不允许为 NULL
 * @param initial_state     初始状态估计值，一般取第一个采样值
 * @param initial_error     初始估计误差协方差 P0，常取 1
 * @param process_noise     过程噪声协方差 Q
 * @param measurement_noise 测量噪声协方差 R
 * @note 纯赋值操作，不分配内存，可重复调用以复位滤波器。
 */
void KalmanInit(KalmanFilter *kf, double initial_state, double initial_error,
                double process_noise, double measurement_noise);

/**
 * @brief 送入一个新采样值，执行一次预测+校正迭代
 * @param kf          滤波器实例指针，不允许为 NULL
 * @param measurement 本次采样到的原始测量值
 * @param first       非 0 表示这是第一个采样点，内部会用该值自动做初始化
 *                    （Q=0.4、R=0.03 为本项目经验调参值）；后续调用传 0
 * @note 迭代完成后滤波结果保存在 kf->v 中，由调用方读取。
 *       本函数非线程安全，同一实例只应在一个线程/定时器上下文里更新。
 */
void KalmanUpdate(KalmanFilter *kf, double measurement, int first);

#endif

