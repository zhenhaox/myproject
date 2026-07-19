/**
 * @file soc.h
 * @brief SoC / 系统信息查询接口：芯片型号、内核版本、系统版本
 * @note  数据来源为 /proc/device-tree/compatible 与 /proc/version；
 *        返回值均为 strdup 出来的堆内存，调用者用完必须 free()
 */

#ifndef __SOC_H__
#define __SOC_H__

/**
 * @brief  读取设备树 compatible 字符串（芯片/板级兼容名列表）
 * @return 堆分配字符串，失败返回 NULL；调用者负责 free()
 */
char *get_compatible_name(void);

/**
 * @brief  从 compatible 字符串中解析出格式化的芯片型号（如 RK1126）
 * @param  buf 原始 compatible 内容
 * @return 堆分配字符串，调用者负责 free()
 */
char *get_soc_name(char *buf);

/**
 * @brief  提取内核版本号（如 "6.1.75"）
 * @param  buf 为 NULL 时自动读取 /proc/version，否则解析 buf
 * @return 堆分配字符串，调用者负责 free()
 */
char *get_kernel_version(char *buf);

/**
 * @brief  读取 /proc/version 整行（完整内核版本描述）
 * @return 堆分配字符串，失败返回 NULL；调用者负责 free()
 */
char *get_system_version(void);

#endif

