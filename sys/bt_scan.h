/**
 * @file bt_scan.h
 * @brief 蓝牙设备扫描模块（BlueZ D-Bus，支持经典蓝牙 + BLE）
 * @note  扫描在工作线程中执行，模块内部不依赖 LVGL，
 *        UI 侧通过快照接口轮询结果
 */

#ifndef BT_SCAN_H_
#define BT_SCAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define BT_SCAN_MAX_DEVICES 32  /**< 单次扫描最多记录的设备数 */

/** 扫描到的设备信息 */
typedef struct {
    char addr[18];  /**< MAC 地址，形如 "AA:BB:CC:DD:EE:FF" */
    char name[64];  /**< 设备名，未知时为 "Unknown" */
    int16_t rssi;   /**< 信号强度，0 表示未知 */
} bt_device_t;

/**
 * @brief  启动一次蓝牙扫描（创建后台线程，约 12 秒后自动结束）
 * @return 0=成功启动，-1=已在扫描中或线程创建失败
 * @note   仅表示请求被受理（API Result），扫描结果通过
 *         bt_scan_get_devices() 轮询获取
 */
int bt_scan_start(void);

/**
 * @brief  停止扫描并等待工作线程退出（阻塞，通常 < 300ms）
 * @note   与 bt_scan_start 配对调用；未在扫描时调用安全（空操作）
 */
void bt_scan_stop(void);

/**
 * @brief  查询扫描线程是否仍在运行
 * @return true=扫描进行中
 */
bool bt_scan_is_running(void);

/**
 * @brief  获取当前已发现设备的快照（mutex 保护下的拷贝）
 * @param  out 输出数组
 * @param  max 数组容量
 * @return 实际拷贝的设备数量
 */
int bt_scan_get_devices(bt_device_t *out, int max);

/**
 * @brief  获取最后一次扫描的错误信息
 * @return NULL=无错误；否则为静态字符串（无需释放）
 */
const char *bt_scan_get_error(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_SCAN_H_ */
