/**
 * @file lv_port_init.h
 * @brief LVGL 移植层总初始化入口声明。main() 调用 lv_port_init()
 *        即可完成 LVGL 内核、DRM 显示、触摸输入的全部底层初始化。
 */

#ifndef LV_PORT_INIT_H
#define LV_PORT_INIT_H

/**
 * @brief  LVGL 底层一键初始化：内核 + 显示 + 输入
 * @param  width    屏幕水平分辨率
 * @param  height   屏幕垂直分辨率
 * @param  rotation 显示旋转角度（0/90/180/270）
 * @note   必须在任何 LVGL API 之前调用
 */
void lv_port_init(int width, int height, int rotation);

#endif

