/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/**
* @file    setup_scr_screen_1.c
* @brief   蓝牙扫描页（screen_1）界面搭建代码，由 NXP GUI Guider 生成后人工适配。
*          创建扫描页全部控件：背景、旋转雷达动画图、返回按钮、半透明面板、
*          “打印机列表”/“正在搜索...”标签、蓝牙设备列表。
*          雷达旋转动画与蓝牙扫描的启动在文件尾部的人工代码段完成；
*          扫描由 sys/bt_scan.c 的工作线程经 BlueZ D-Bus（GDBus）执行，
*          结果由 lv_timer 轮询刷新进列表（见 custom.c 的 bt_scan_ui_start()）。
* @note    LVGL 非线程安全：本文件所有函数只允许在 LVGL 主线程中执行。
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



/**
* @brief  搭建蓝牙扫描页（screen_1）：创建控件、启动雷达动画并启动蓝牙扫描。
* @param  ui  全局 UI 结构体，各控件句柄保存在其中。
* @note   离开本页（返回主屏）时必须配对调用 bt_scan_ui_stop() 停扫描并删除
*         轮询定时器（已在返回按钮事件中处理），否则后台线程和定时器会泄漏。
*/
void setup_scr_screen_1(lv_ui *ui)
{
    //Write codes screen_1
    ui->screen_1 = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_1, 720, 1280);
    lv_obj_set_scrollbar_mode(ui->screen_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_cont_1
    // 整屏容器当“背景板”：垫在最底层，下面 lv_img_create_cover() 再贴满屏壁纸
    ui->screen_1_cont_1 = lv_obj_create(ui->screen_1);
    lv_obj_set_pos(ui->screen_1_cont_1, 0, 0);
    lv_obj_set_size(ui->screen_1_cont_1, 720, 1280);
    lv_obj_set_scrollbar_mode(ui->screen_1_cont_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_1_cont_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_1_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_1_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_1_cont_1, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_1_cont_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_1_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_1_cont_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_1_cont_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_1_cont_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_1_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_1_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_1_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_1_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_img_create_cover(ui->screen_1_cont_1, "/mnt/sdcard/icons/astronaut_720x1280.png", 720, 1280);
    lv_obj_set_style_shadow_width(ui->screen_1_cont_1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_scanbt_img
    // 雷达扫描图：文件尾部人工代码会把旋转中心改到图中心，并启动无限旋转动画
    ui->screen_1_scanbt_img = lv_img_create(ui->screen_1);
    lv_obj_add_flag(ui->screen_1_scanbt_img, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src_cover(ui->screen_1_scanbt_img, "/mnt/sdcard/icons/scan_200x200.png", 200, 200);
    lv_img_set_pivot(ui->screen_1_scanbt_img, 50,50);
    lv_img_set_angle(ui->screen_1_scanbt_img, 0);
    lv_obj_set_pos(ui->screen_1_scanbt_img, 264, 47);
    lv_obj_set_size(ui->screen_1_scanbt_img, 200, 200);

    //Write style for screen_1_scanbt_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_1_scanbt_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_1_scanbt_img, 191, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_1_scanbt_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_1_scanbt_img, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_cancer_btn
    // “取消/返回”按钮：透明底，用一张 cancer 图标铺满当按钮皮肤，点击返回主屏
    ui->screen_1_cancer_btn = lv_btn_create(ui->screen_1);
    ui->screen_1_cancer_btn_label = lv_label_create(ui->screen_1_cancer_btn);
    lv_label_set_text(ui->screen_1_cancer_btn_label, "");
    lv_label_set_long_mode(ui->screen_1_cancer_btn_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(ui->screen_1_cancer_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_pad_all(ui->screen_1_cancer_btn, 0, LV_STATE_DEFAULT);
    lv_obj_set_width(ui->screen_1_cancer_btn_label, LV_PCT(100));
    lv_obj_set_pos(ui->screen_1_cancer_btn, 572, 47);
    lv_obj_set_size(ui->screen_1_cancer_btn, 100, 100);

    //Write style for screen_1_cancer_btn, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_1_cancer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui->screen_1_cancer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_1_cancer_btn, 5, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_1_cancer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_img_create_cover(ui->screen_1_cancer_btn, "/mnt/sdcard/icons/cancer_100x100.png", 100, 100);
    lv_obj_set_style_bg_img_opa(ui->screen_1_cancer_btn, 132, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_recolor_opa(ui->screen_1_cancer_btn, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_1_cancer_btn, lv_color_hex(0x843d3d), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_1_cancer_btn, &lv_font_SourceHanSerifSC_Regular_52, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_1_cancer_btn, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_1_cancer_btn, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_brid_img
    ui->screen_1_brid_img = lv_img_create(ui->screen_1);
    lv_obj_add_flag(ui->screen_1_brid_img, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src_cover(ui->screen_1_brid_img, "/mnt/sdcard/icons/bird_144x159.png", 144, 159);
    lv_img_set_pivot(ui->screen_1_brid_img, 50,50);
    lv_img_set_angle(ui->screen_1_brid_img, 0);
    lv_obj_set_pos(ui->screen_1_brid_img, 82, 109);
    lv_obj_set_size(ui->screen_1_brid_img, 144, 159);

    //Write style for screen_1_brid_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_img_recolor_opa(ui->screen_1_brid_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_img_opa(ui->screen_1_brid_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_1_brid_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui->screen_1_brid_img, true, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_cont_2
    // 半透明圆角面板：垫在“打印机列表”文字和设备列表背后，压住壁纸提升可读性
    ui->screen_1_cont_2 = lv_obj_create(ui->screen_1);
    lv_obj_set_pos(ui->screen_1_cont_2, 62, 289);
    lv_obj_set_size(ui->screen_1_cont_2, 597, 631);
    lv_obj_set_scrollbar_mode(ui->screen_1_cont_2, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_1_cont_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_1_cont_2, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui->screen_1_cont_2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui->screen_1_cont_2, lv_color_hex(0x2195f6), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui->screen_1_cont_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_1_cont_2, 31, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_1_cont_2, 170, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_1_cont_2, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_1_cont_2, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_1_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_1_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_1_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_1_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_1_cont_2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_printer_label
    ui->screen_1_printer_label = lv_label_create(ui->screen_1);
    lv_label_set_text(ui->screen_1_printer_label, "打印机列表");
    lv_label_set_long_mode(ui->screen_1_printer_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_1_printer_label, 111, 328);
    lv_obj_set_size(ui->screen_1_printer_label, 139, 29);

    //Write style for screen_1_printer_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_1_printer_label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_1_printer_label, &lv_font_SourceHanSerifSC_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_1_printer_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_1_printer_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_1_printer_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_scan_status_label
    ui->screen_1_scan_status_label = lv_label_create(ui->screen_1);
    lv_label_set_text(ui->screen_1_scan_status_label, "正在搜索...");
    lv_label_set_long_mode(ui->screen_1_scan_status_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->screen_1_scan_status_label, 270, 328);
    lv_obj_set_size(ui->screen_1_scan_status_label, 360, 29);

    //Write style for screen_1_scan_status_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_text_color(ui->screen_1_scan_status_label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_1_scan_status_label, &lv_font_SourceHanSerifSC_Regular_24, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_1_scan_status_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_1_scan_status_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_device_list
    // 蓝牙设备列表：这里只建空列表，扫描到的设备条目由轮询定时器动态添加（custom.c）
    ui->screen_1_device_list = lv_list_create(ui->screen_1);
    lv_obj_set_pos(ui->screen_1_device_list, 82, 370);
    lv_obj_set_size(ui->screen_1_device_list, 557, 530);
    lv_obj_set_scrollbar_mode(ui->screen_1_device_list, LV_SCROLLBAR_MODE_AUTO);

    //Write style for screen_1_device_list, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_1_device_list, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_1_device_list, 16, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_1_device_list, 220, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_1_device_list, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_1.
    // 把旋转中心改到图片中心（默认 pivot 在左上角），否则雷达图会绕角“甩”而不是原地转
    lv_img_set_pivot(ui->screen_1_scanbt_img,
                     lv_obj_get_width(ui->screen_1_scanbt_img) / 2,
                     lv_obj_get_height(ui->screen_1_scanbt_img) / 2);

    // 雷达旋转动画：LVGL 动画器在主循环里周期性回调 lv_img_set_angle 改图片角度
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui->screen_1_scanbt_img);  // 你的雷达图片
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_angle);
    lv_anim_set_values(&a, 0, 3600);   // 角度单位 0.1°：0~3600 即转一整圈
    lv_anim_set_time(&a, 2000);        // 转一圈耗时 2 秒
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);

    /* 进入扫描页自动启动蓝牙扫描，结果由轮询定时器刷新到列表 */
    // 内部会拉起 GDBus 扫描工作线程，并建 500ms lv_timer 把新设备刷进上面的列表；
    // 工作线程只写共享数据、不直接碰 LVGL 对象，界面刷新统一在 LVGL 主线程做
    bt_scan_ui_start();

    //Update current screen layout.
    // 立即重算一次布局，保证之后读取坐标/尺寸的代码拿到的是最新值
    lv_obj_update_layout(ui->screen_1);

    //Init events for screen.
    // 注册本屏控件的事件回调（返回按钮里会调 bt_scan_ui_stop()），见 events_init.c
    events_init_screen_1(ui);
}
