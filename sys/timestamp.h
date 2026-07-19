/**
 * @file timestamp.h
 * @brief 毫秒/微秒时间戳与简易打点计时接口
 * @note  基于 gettimeofday（墙上时钟），用于打印日志、粗测代码段耗时
 */

#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <stdint.h>

/**
 * @brief  获取当前墙上时间的毫秒数
 * @return 自 Unix 纪元起的毫秒值（低位会周期性回绕，只适合做差值）
 */
uint32_t clock_ms(void);

/**
 * @brief  获取当前墙上时间的微秒数
 * @return 自 Unix 纪元起的微秒值
 */
uint64_t clock_us(void);

/**
 * @brief  打点计时：start=1 记录起点，start=0 打印与起点的时间差
 * @param  fmt   起点处打印的标签字符串
 * @param  index 计时槽编号（0~127），同一编号配对使用
 * @param  start 非 0=打起点；0=打终点并输出 cost
 * @note   计时槽为静态数组，跨调用保持，故非线程安全
 */
void timestamp(char *fmt, int index, int start);

#endif

