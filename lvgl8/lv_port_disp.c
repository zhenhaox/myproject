/**
 * @file lv_port_disp.c
 * @brief LVGL 显示移植层：根据编译开关把 LVGL 对接到具体显示后端。
 *        支持三种后端（由 lv_drv_conf.h 中的 USE_DRM / USE_SDL_GPU / USE_RKADK 选择）：
 *          - DRM/KMS：Linux 内核显示子系统直驱屏幕（/dev/dri，本工程目标板所用方式）；
 *          - SDL：PC 模拟窗口调试界面用；
 *          - RKADK：Rockchip 摄像头/显示通路（rockchip 官方 SDK 场景）。
 *        同一时刻只会启用其中一种。
 */

#include <stdlib.h>
#include <lvgl/lvgl.h>
#include <lv_drivers/display/drm.h>
#include <lv_drivers/rkadk/rkadk.h>
#include <lv_drivers/sdl/sdl_gpu.h>

/**
 * @brief  初始化 LVGL 显示后端并完成屏幕旋转配置
 * @param  hor_res 屏幕水平分辨率（本机 720）
 * @param  ver_res 屏幕垂直分辨率（本机 1280，竖屏）
 * @param  rot     显示旋转角度，仅支持 0/90/180/270，其余值仅打印错误日志并忽略
 * @note   LVGL 8 的软件旋转（disp_drv.rotated / sw_rotate）在刷新回调里做像素搬运，
 *         会明显占用 CPU；本工程屏幕旋转在 DRM 驱动侧配合处理。
 */
void lv_port_disp_init(lv_coord_t hor_res, lv_coord_t ver_res, int rot)
{
    lv_disp_rot_t lvgl_rot = LV_DISP_ROT_NONE;
    lv_disp_t *disp;

    // 把应用层的角度值翻译成 LVGL 枚举，非法角度只报错不退出，
    // 保持默认不旋转，避免参数错误直接导致黑屏
    switch (rot)
    {
    case 0:
        lvgl_rot = LV_DISP_ROT_NONE;
        break;
    case 90:
        lvgl_rot = LV_DISP_ROT_90;
        break;
    case 180:
        lvgl_rot = LV_DISP_ROT_180;
        break;
    case 270:
        lvgl_rot = LV_DISP_ROT_270;
        break;
    default:
        LV_LOG_ERROR("Unsupported rotation %d", rot);
        break;
    }

#if USE_DRM
    // 目标板实际路径：初始化 DRM/KMS 显示驱动（打开 /dev/dri/cardX，
    // 申请双缓冲 dumb buffer 并注册到 LVGL；双缓冲可避免刷新时屏幕撕裂）
    drm_disp_drv_init(hor_res, ver_res, rot);
#endif

#if USE_SDL_GPU
    // PC 模拟路径：SDL 窗口模拟屏，便于在开发机上先调界面再烧录
    static lv_disp_drv_t disp_drv;  // 必须 static：LVGL 注册后长期引用该结构体
    monitor_init();
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN);
    disp_drv.rotated = lvgl_rot;
    sdl_disp_drv_init(&disp_drv, hor_res, ver_res);

    disp = lv_disp_drv_register(&disp_drv);
#endif

#if USE_RKADK
    // Rockchip 多媒体通路路径：显示与摄像头视频流共用 RKADK 通道
    rkadk_disp_drv_init(lvgl_rot);
#endif
}

