/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/**
 * @file events_init.c
 * @brief 界面事件回调实现：弹窗显隐、两个 screen 之间的切换调度
 *
 * 屏幕切换不直接在点击回调里做，而是创建一个 50ms 的一次性 lv_timer
 * 延迟执行：事件回调返回后 LVGL 才处理对象删除/重绘，此时再切屏可避免
 * 在事件派发过程中删除事件源对象（类似 MCU 里"中断里只置标志，主循环
 * 里再处理"的延迟处理思路）。
 */

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"
#include "custom.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

/* 屏幕切换在事件回调结束后延迟执行，避免在事件源对象被删除的过程中切换 */
/**
 * @brief 切到蓝牙扫描页 screen_1 的一次性定时器回调
 * @param timer 定时器对象，进入即删（只执行一次）
 * @note  切走前先把 guider_ui.screen_clock 置 NULL：旧 screen 即将被
 *        LVGL 自动删除，而秒定时器 screen_clock_timer 仍在运行，
 *        置空后它因判空跳过刷新，避免写已释放的 label（野指针）。
 */
static void load_screen_1_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(timer);
    guider_ui.screen_clock = NULL;  // 关键：防止时钟定时器访问野指针
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_1, &guider_ui.screen_1_del,
                          &guider_ui.screen_del, setup_scr_screen_1,
                          LV_SCR_LOAD_ANIM_NONE, 0, 0, true, false);
}

/**
 * @brief 切回主界面 screen 的一次性定时器回调
 * @param timer 定时器对象，进入即删（只执行一次）
 * @note  先调 bt_scan_ui_stop() 停扫描线程、删轮询定时器，再切屏，
 *        否则 screen_1 删除后台线程刷列表会访问已释放的控件。
 */
static void load_screen_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(timer);
    bt_scan_ui_stop();  // 离开扫描页前停止扫描线程并删除轮询定时器
    ui_load_scr_animation(&guider_ui, &guider_ui.screen, &guider_ui.screen_del,
                          &guider_ui.screen_1_del, setup_scr_screen,
                          LV_SCR_LOAD_ANIM_OVER_LEFT, 0, 0, true, true);
}

/**
 * @brief 主界面"断开连接"图片的点击回调：弹出打印机选择弹窗
 * @param e LVGL 事件对象
 */
static void screen_disconnect_image_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_clear_flag(guider_ui.screen_printer_popup, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief 主界面"添加打印机"按钮回调：延迟 50ms 切到蓝牙扫描页
 * @param e LVGL 事件对象
 * @note  用一次性 lv_timer 延迟切屏，原因见文件头说明
 */
static void screen_addprinter_btn_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_timer_create(load_screen_1_timer_cb, 50, NULL);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief 弹窗"取消"按钮回调：隐藏打印机选择弹窗
 * @param e LVGL 事件对象
 */
static void screen_cancer_btn_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_add_flag(guider_ui.screen_printer_popup, LV_OBJ_FLAG_HIDDEN);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief 为主界面 screen 的控件统一注册事件回调
 * @param ui 全局 UI 结构体指针
 * @note  LV_EVENT_ALL 注册全部事件码，回调内部再用 switch 过滤点击事件；
 *        user_data 传 ui 指针，回调里可用 lv_event_get_user_data() 取回
 */
void events_init_screen (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_disconnect_image, screen_disconnect_image_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_addprinter_btn, screen_addprinter_btn_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->screen_cancer_btn, screen_cancer_btn_event_handler, LV_EVENT_ALL, ui);
}

/**
 * @brief 扫描页"取消"按钮回调：延迟 50ms 切回主界面
 * @param e LVGL 事件对象
 * @note  实际切换在 load_screen_timer_cb 里做，那里会先停蓝牙扫描
 */
static void screen_1_cancer_btn_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_timer_create(load_screen_timer_cb, 50, NULL);
        break;
    }
    default:
        break;
    }
}

/**
 * @brief 为蓝牙扫描页 screen_1 的控件统一注册事件回调
 * @param ui 全局 UI 结构体指针
 */
void events_init_screen_1 (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->screen_1_cancer_btn, screen_1_cancer_btn_event_handler, LV_EVENT_ALL, ui);
}


/**
 * @brief 全局事件初始化入口（GUI Guider 生成的保留接口）
 * @param ui 全局 UI 结构体指针
 * @note  本项目各 screen 的事件在各自的 events_init_xxx() 中注册，
 *        此函数留空仅为兼容生成代码的调用约定
 */
void events_init(lv_ui *ui)
{

}
