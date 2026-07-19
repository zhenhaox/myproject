/**
 * @file key.h
 * @brief 物理按键(evdev)驱动的对外接口声明，与 key.c 配套。
 *        本工程默认 USE_KEY=0，接口不参与编译，仅作预留。
 */

#ifndef KEY_H
#define KEY_H

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------
 * Mouse or touchpad as evdev interface (for Linux based systems)
 *------------------------------------------------*/
// 按键功能总开关：0=不编译 key.c 相关内容（本工程用触摸，未启用物理按键）
#define USE_KEY           0

#if USE_KEY
// 按键对应的输入设备节点；event 编号由内核设备树/驱动枚举顺序决定，
// 可用 cat /proc/bus/input/devices 确认实际编号
#  define KEY_NAME   "/dev/input/event2"

#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the evdev
 * @brief  打开按键设备节点并设为非阻塞模式（详见 key.c 实现注释）
 * @return 0: 成功; -1: 失败
 */
int key_init(void);

/**
 * Get the current position and state of the evdev
 * @brief  LVGL 输入设备读回调：把内核按键事件翻译成 LVGL 键值
 * @param  drv  调用本回调的输入设备驱动对象
 * @param  data store the evdev data here（输出参数：回写键值与状态）
 * @return false: because the points are not buffered, so no more data to be read
 */
void key_read(lv_indev_drv_t *drv, lv_indev_data_t *data);


/**********************
 *      MACROS
 **********************/

#endif /* USE_KEY */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* KEY_H */
