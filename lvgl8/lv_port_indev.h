
/**
 * @file lv_port_indev.h
 * @brief LVGL 输入设备移植层对外接口：输入设备初始化、按键组(group)创建/销毁，
 *        以及（可选）传感器驱动对象的获取。界面切换时配合 group 接口管理按键焦点。
 */

/*Copy this file as "lv_port_indev.h" and set this value to "1" to enable content*/
#if 1

// 头文件保护宏名沿用官方模板命名(TEMPL)，不影响功能
#ifndef LV_PORT_INDEV_TEMPL_H
#define LV_PORT_INDEV_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "evdev.h"

/**
 * @brief  初始化并注册全部输入设备（触摸/按键/SDL 鼠标，按编译开关裁剪）
 * @param  rot 输入坐标旋转角度（0/90/180/270），须与屏幕旋转一致
 */
void lv_port_indev_init(int rot);

/**
 * @brief  创建按键组并接管按键焦点（新界面获得焦点导航能力）
 * @return 新建的 lv_group_t 指针，销毁时必须传回 lv_port_indev_group_destroy()
 */
lv_group_t *lv_port_indev_group_create(void);

/**
 * @brief  销毁按键组并把按键焦点回退给上一个 group
 * @param  group 待销毁的 group
 */
void lv_port_indev_group_destroy(lv_group_t *group);

#if USE_SENSOR
/**
 * @brief  获取光传感器驱动对象（未注册进 LVGL，应用层自行周期读取）
 * @return 光传感器 lv_indev_drv_t 指针
 */
lv_indev_drv_t *lv_port_indev_get_lsensor_drv(void);

/**
 * @brief  获取接近传感器驱动对象（未注册进 LVGL，应用层自行周期读取）
 * @return 接近传感器 lv_indev_drv_t 指针
 */
lv_indev_drv_t *lv_port_indev_get_psensor_drv(void);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_INDEV_TEMPL_H*/

#endif /*Disable/Enable content*/
