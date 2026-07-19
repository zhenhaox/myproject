/**
 * @file lv_port_disp.h
 * @brief LVGL 显示移植层对外接口：声明 lv_port_disp_init()。
 *        由 lv_port_init() 统一调用，应用层一般不直接使用。
 */

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  初始化 LVGL 显示后端（DRM/SDL/RKADK 三选一，由编译开关决定）
 * @param  hor_res 屏幕水平分辨率
 * @param  ver_res 屏幕垂直分辨率
 * @param  rot     显示旋转角度：0/90/180/270
 */
void lv_port_disp_init(lv_coord_t hor_res, lv_coord_t ver_res, int rot);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_H*/

