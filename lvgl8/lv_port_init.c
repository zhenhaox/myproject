/**
 * @file lv_port_init.c
 * @brief LVGL 移植层总初始化入口：依次完成 LVGL 内核初始化、
 *        显示后端初始化(DRM/KMS)、输入设备注册(evdev 触摸)。
 *        main() 只需调用 lv_port_init() 一个函数即可拉起整个 GUI 底层。
 */

#include <lvgl/lvgl.h>
#include <lvgl/lv_conf.h>

#include "lv_port_disp.h"
#include "lv_port_indev.h"

/* 0, 90, 180, 270 */
// 触摸坐标相对显示画面的额外旋转补偿：
// 触摸屏本身的安装方向与显示旋转不一致时，改这里做固定偏移，不用动应用参数
static int g_indev_rotation = 0;

/**
 * @brief  LVGL 底层一键初始化：内核 + 显示 + 输入
 * @param  width    屏幕水平分辨率（本机 720）
 * @param  height   屏幕垂直分辨率（本机 1280，竖屏）
 * @param  rotation 显示旋转角度（0/90/180/270）
 * @note   必须在任何 LVGL API 之前调用；lv_init() 只初始化内核数据结构，
 *         显示/输入设备的注册分别由后两步完成，顺序不可调换
 *         （输入初始化里要取默认显示设备，因此显示必须先就绪）。
 */
void lv_port_init(int width, int height, int rotation)
{
    lv_init();

    lv_port_disp_init(width, height, rotation);
    lv_port_indev_init(g_indev_rotation + rotation);
}

