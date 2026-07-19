/**
 * @file cpu.h
 * @brief 整机 CPU 占用率统计接口
 * @note  供信息页/状态栏按固定周期调用，数据来源于 /proc/stat
 */

#ifndef __CPU_H__
#define __CPU_H__

/**
 * @brief  获取距上次调用这段时间内的整机 CPU 占用率
 * @return 百分比（0~100）
 * @note   调用间隔即统计窗口，应按固定周期（如 1s）调用；非可重入
 */
double get_cpu_usage(void);

#endif


