/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/**
* @file    setup_scr_screen.c
* @brief   主屏（首页）界面搭建代码，由 NXP GUI Guider 生成后人工适配。
*          创建主屏全部控件：背景容器、时钟/日期标签、蓝牙连接状态图标、
*          “校准时间”/“打印图片”按钮、“我的打印机”弹窗（内含“添加打印机”入口）。
*          本文件只管“界面长什么样”：控件点击行为在 events_init.c 注册，
*          时钟走时逻辑在 custom.c 的 screen_clock_timer() 中。
*          屏幕固定 720x1280 竖屏，坐标原点在左上角，Y 轴向下。
* @note    LVGL 非线程安全：本文件所有函数只允许在 LVGL 主线程
*          （调用 lv_timer_handler() 的线程）中执行。
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



/* 界面时钟的软件计时值，由 custom.c 的 screen_clock_timer() 每秒自增并进位，
   再格式化刷新到时钟标签；初值与时钟标签初始文本 "11:25:50" 一一对应 */
int screen_clock_min_value = 25;  /**< 时钟“分”当前值 */
int screen_clock_hour_value = 11; /**< 时钟“时”当前值 */
int screen_clock_sec_value = 50;  /**< 时钟“秒”当前值 */
/**
* @brief  搭建主屏（screen）界面：创建全部控件并设置样式，最后注册事件回调。
* @param  ui  全局 UI 结构体，各控件句柄保存在其中，供事件回调和其他模块访问。
* @note   每次切回本屏都会重新调用本函数重建整棵控件树（旧树由切页逻辑删除），
*         因此函数内用 static 标志保证 1 秒时钟定时器只创建一次，避免定时器叠加。
*/
void setup_scr_screen(lv_ui *ui)
{
    //Write codes screen
    // 父对象传 NULL：创建的是“屏幕”根对象，可交给 lv_scr_load() 整屏切换
    ui->screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen, 720, 1280);   // 与 MIPI 屏物理分辨率一致
    lv_obj_set_scrollbar_mode(ui->screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_cont_1
    // 整屏容器当“背景板”：不透明垫在最底层，下面 lv_img_create_cover() 再贴满屏壁纸
    ui->screen_cont_1 = lv_obj_create(ui->screen);
    lv_obj_set_pos(ui->screen_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_cont_1, 720, 1280);
    lv_obj_set_scrollbar_mode(ui->screen_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_cont_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_cont_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_cont_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    // 从 SD 卡读 PNG 壁纸并 cover 裁剪填满容器（自定义辅助函数，见 custom.c）；
    // 图片存文件系统、运行时解码，≠ MCU 把图片数组编译进 Flash
    lv_img_create_cover(ui->screen_cont_1, "/mnt/sdcard/icons/ocean_720x1280.png", 720, 1280);
    lv_obj_set_style_shadow_width(ui->screen_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_clock
    // static 局部变量只初始化一次：界面被反复重建时，保证下面的 1 秒定时器只建一次
    static bool screen_clock_timer_enabled = false;
    ui->screen_clock = lv_label_create(ui->screen);
    lv_label_set_text(ui->screen_clock, "11:25:50");
    if (!screen_clock_timer_enabled) {
        // lv_timer ≈ MCU 的软件定时器，由 lv_timer_handler() 周期轮询触发；
        // 回调 screen_clock_timer()（custom.c）每秒累加时分秒变量并刷新本标签
        lv_timer_create(screen_clock_timer, 1000, NULL);
        screen_clock_timer_enabled = true;
    }
    lv_obj_set_pos(ui->screen_clock, 85, 26);
    lv_obj_set_size(ui->screen_clock, 147, 46);

    //Write style for screen_clock, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_radius(ui->screen_clock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_clock, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_clock, &lv_font_Alatsi_Regular_35, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_clock, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_clock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_clock, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_clock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_clock, 7, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_clock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_clock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_clock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_clock, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_datetext
    ui->screen_datetext = lv_label_create(ui->screen);
    lv_label_set_text(ui->screen_datetext, "2023/07/31");
    lv_obj_set_style_text_align(ui->screen_datetext, LV_TEXT_ALIGN_CENTER, 0);
    // 日期文本也可点击：回调是 custom.c 里的占位函数，预留给“点击进入日期设置”
    lv_obj_add_flag(ui->screen_datetext, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui->screen_datetext, screen_datetext_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_pos(ui->screen_datetext, 22, 88);
    lv_obj_set_size(ui->screen_datetext, 226, 49);

    //Write style for screen_datetext, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_datetext, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_datetext, &lv_font_Alatsi_Regular_35, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_datetext, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_datetext, 4, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_datetext, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_datetext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_datetext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_datetext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_datetext, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_datetext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_datetext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_datetext, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_disconnect_image
    // “未连接”状态图标：与下方 screen_img_2（“已连接”图标）坐标相同、互斥显隐
    ui->screen_disconnect_image = lv_img_create(ui->screen);
    lv_obj_add_flag(ui->screen_disconnect_image, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src_cover(ui->screen_disconnect_image, "/mnt/sdcard/icons/noconnect_100x100.png", 100, 100);
    lv_img_set_pivot(ui->screen_disconnect_image, 50,50);
    lv_img_set_angle(ui->screen_disconnect_image, 0);
    lv_obj_set_pos(ui->screen_disconnect_image, 565, 34);
    lv_obj_set_size(ui->screen_disconnect_image, 100, 100);

    //Write style for screen_disconnect_image, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_disconnect_image, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_disconnect_image, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_disconnect_image, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_disconnect_image, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_adjust_btn
    // “校准时间”按钮；中文能显示靠 SourceHanSerifSC 字体——由 FreeType 运行时
    // 从 /usr/share/fonts 加载（widgets_init.c 的 gui_guider_get_font()）
    ui->screen_adjust_btn = lv_btn_create(ui->screen);
    ui->screen_adjust_btn_label = lv_label_create(ui->screen_adjust_btn);
    lv_label_set_text(ui->screen_adjust_btn_label, "校准时间");
    lv_label_set_long_mode(ui->screen_adjust_btn_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_adjust_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_adjust_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_adjust_btn_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_adjust_btn, 82, 908);
    lv_obj_set_size(ui->screen_adjust_btn, 188, 104);

    //Write style for screen_adjust_btn, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_adjust_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_adjust_btn, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_adjust_btn, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_adjust_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_adjust_btn, 19, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_adjust_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_adjust_btn, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_adjust_btn, &lv_font_SourceHanSerifSC_Regular_34, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_adjust_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_adjust_btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_printer_btn
    ui->screen_printer_btn = lv_btn_create(ui->screen);
    ui->screen_printer_btn_label = lv_label_create(ui->screen_printer_btn);
    lv_label_set_text(ui->screen_printer_btn_label, "打印图片");
    lv_label_set_long_mode(ui->screen_printer_btn_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_printer_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_printer_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_printer_btn_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_printer_btn, 416, 908);
    lv_obj_set_size(ui->screen_printer_btn, 188, 104);

    //Write style for screen_printer_btn, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_printer_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_printer_btn, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_printer_btn, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_printer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_printer_btn, 19, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_printer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_printer_btn, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_printer_btn, &lv_font_SourceHanSerifSC_Regular_34, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_printer_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_printer_btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_printer_popup
    // “我的打印机”弹窗：先创建好但默认隐藏，点“打印图片”按钮时只切换显隐标志，
    // 避免每次开关都反复创建/销毁整棵弹窗控件树（含图片解码）的开销
    ui->screen_printer_popup = lv_obj_create(ui->screen);
    lv_obj_set_pos(ui->screen_printer_popup, 0, 544);
    lv_obj_set_size(ui->screen_printer_popup, 720, 730);
    lv_obj_set_scrollbar_mode(ui->screen_printer_popup, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui->screen_printer_popup, LV_OBJ_FLAG_HIDDEN);

    //Write style for screen_printer_popup, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_printer_popup, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_printer_popup, 32, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_printer_popup, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_printer_popup, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_printer_popup, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_printer_popup, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_printer_popup, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_img_create_cover(ui->screen_printer_popup, "/mnt/sdcard/icons/beach_720x730.png", 720, 730);
    lv_obj_set_style_shadow_width(ui->screen_printer_popup, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_myprinter_label
    ui->screen_myprinter_label = lv_label_create(ui->screen_printer_popup);
    lv_label_set_text(ui->screen_myprinter_label, "我的打印机");
    lv_label_set_long_mode(ui->screen_myprinter_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_myprinter_label, 24, 36);
    lv_obj_set_size(ui->screen_myprinter_label, 215, 56);

    //Write style for screen_myprinter_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_myprinter_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_myprinter_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_myprinter_label, lv_color_hex(0x0f0e0f), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_myprinter_label, &lv_font_SourceHanSerifSC_Regular_30, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_myprinter_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_myprinter_label, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_myprinter_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_myprinter_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_myprinter_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_myprinter_label, 8, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_myprinter_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_myprinter_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_myprinter_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_myprinter_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_addprinter_btn
    ui->screen_addprinter_btn = lv_btn_create(ui->screen_printer_popup);
    ui->screen_addprinter_btn_label = lv_label_create(ui->screen_addprinter_btn);
    lv_label_set_text(ui->screen_addprinter_btn_label, "添加打印机");
    lv_label_set_long_mode(ui->screen_addprinter_btn_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_addprinter_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_addprinter_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_addprinter_btn_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_addprinter_btn, 220, 326);
    lv_obj_set_size(ui->screen_addprinter_btn, 258, 54);

    //Write style for screen_addprinter_btn, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_addprinter_btn, 95, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_addprinter_btn, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_addprinter_btn, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_addprinter_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_addprinter_btn, 17, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_addprinter_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_addprinter_btn, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_addprinter_btn, &lv_font_SourceHanSerifSC_Regular_26, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_addprinter_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_addprinter_btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_cancer_btn
    // 弹窗右上角“×”关闭按钮：背景透明，仅靠文字“×”示意为关闭
    ui->screen_cancer_btn = lv_btn_create(ui->screen_printer_popup);
    ui->screen_cancer_btn_label = lv_label_create(ui->screen_cancer_btn);
    lv_label_set_text(ui->screen_cancer_btn_label, "×");
    lv_label_set_long_mode(ui->screen_cancer_btn_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_cancer_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_cancer_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_cancer_btn_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_cancer_btn, 586, 40);
    lv_obj_set_size(ui->screen_cancer_btn, 100, 50);

    //Write style for screen_cancer_btn, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_cancer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_cancer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_cancer_btn, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_cancer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_cancer_btn, lv_color_hex(0x646392), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_cancer_btn, &lv_font_SourceHanSerifSC_Regular_29, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_cancer_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_cancer_btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_1
    ui->screen_img_1 = lv_img_create(ui->screen);
    lv_obj_add_flag(ui->screen_img_1, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src_cover(ui->screen_img_1, "/mnt/sdcard/icons/time_50x50.png", 50, 50);
    lv_img_set_pivot(ui->screen_img_1, 50,50);
    lv_img_set_angle(ui->screen_img_1, 0);
    lv_obj_set_pos(ui->screen_img_1, 22, 26);
    lv_obj_set_size(ui->screen_img_1, 50, 50);

    //Write style for screen_img_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_img_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_img_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_img_1, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_2
    // “已连接”状态图标：与 screen_disconnect_image 同坐标，默认隐藏，连接成功后才显示
    ui->screen_img_2 = lv_img_create(ui->screen);
    lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src_cover(ui->screen_img_2, "/mnt/sdcard/icons/connect_100x100.png", 100, 100);
    lv_img_set_pivot(ui->screen_img_2, 50,50);
    lv_img_set_angle(ui->screen_img_2, 0);
    lv_obj_set_pos(ui->screen_img_2, 565, 34);
    lv_obj_set_size(ui->screen_img_2, 100, 100);
    lv_obj_add_flag(ui->screen_img_2, LV_OBJ_FLAG_HIDDEN);

    //Write style for screen_img_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_img_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_img_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_img_2, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen.


    //Update current screen layout.
    // 立即重算一次布局，保证之后读取坐标/尺寸的代码拿到的是最新值
    lv_obj_update_layout(ui->screen);

    //Init events for screen.
    // 注册本屏控件的事件回调（跳转扫描页、弹窗显隐等），见 events_init.c
    events_init_screen(ui);
}
