/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


#ifndef EVENTS_INIT_H_
#define EVENTS_INIT_H_
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file events_init.h
 * @brief 界面事件注册接口：为各 screen 的控件挂接事件回调
 *
 * 与 GUI Guider 生成的 events_init 对应：每个 screen 一个 events_init_xxx()
 * 函数，在 setup_scr_xxx() 创建完控件后调用，完成事件回调的绑定。
 */

#include "gui_guider.h"

/**
 * @brief 全局事件初始化入口（GUI Guider 保留接口，本项目为空实现）
 * @param ui 全局 UI 结构体指针
 */
void events_init(lv_ui *ui);

/**
 * @brief 为主界面 screen 的控件注册事件回调
 * @param ui 全局 UI 结构体指针
 * @note  在 setup_scr_screen() 中控件创建完成后调用
 */
void events_init_screen(lv_ui *ui);

/**
 * @brief 为蓝牙扫描页 screen_1 的控件注册事件回调
 * @param ui 全局 UI 结构体指针
 * @note  在 setup_scr_screen_1() 中控件创建完成后调用
 */
void events_init_screen_1(lv_ui *ui);

#ifdef __cplusplus
}
#endif
#endif /* EVENT_CB_H_ */
