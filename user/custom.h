/**
 * @file custom.h
 * @brief 用户自定义逻辑接口：时钟刷新、图片 cover 填充、蓝牙扫描页 UI 胶水
 *
 * 本文件声明的是 GUI Guider 生成代码之外、人工补充的业务逻辑，
 * 供 events_init.c（事件回调）和各 setup_scr_*.c（界面搭建）调用。
 */

#ifndef CUSTOM_H_
#define CUSTOM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief 日期文本的点击事件回调（当前为占位，暂未实现功能）
 * @param e LVGL 事件对象
 */
void screen_datetext_event_handler(lv_event_t *e);

/**
 * @brief 主界面时钟的秒级刷新回调，由 lv_timer 周期触发
 * @param timer 触发本回调的定时器对象
 * @note  直接自增全局时/分/秒变量并刷新 label；不走 RTC，
 *        上电后从初始值开始计时，类似 MCU 里定时器中断软件计时
 */
void screen_clock_timer(lv_timer_t *timer);

/* 图片自适应填充辅助函数 */
/**
 * @brief 让图片以 cover 模式（等比放大、裁掉溢出部分）铺满 w x h 区域
 * @param img  目标 lv_img 对象
 * @param path 图片路径（LVGL 文件系统路径，如 "S:/xxx.png"）
 * @param w    目标宽度（像素）
 * @param h    目标高度（像素）
 * @note  取宽/高两个方向较大的缩放比，保证短边也能铺满；
 *        LVGL 的 zoom 是 1/256 精度的定点数，256 表示原始大小
 */
void lv_img_set_src_cover(lv_obj_t *img, const char *path, lv_coord_t w, lv_coord_t h);

/**
 * @brief 在 parent 内新建一张 cover 模式铺满的图片，并压到最底层当背景
 * @param parent 父对象（通常是某个 screen）
 * @param path   图片路径
 * @param w      目标宽度（像素）
 * @param h      目标高度（像素）
 * @return 创建成功的 lv_img 对象指针
 */
lv_obj_t *lv_img_create_cover(lv_obj_t *parent, const char *path, lv_coord_t w, lv_coord_t h);

/* 蓝牙扫描页胶水：进入扫描页时启动，离开扫描页时必须调用 stop */
/**
 * @brief 启动蓝牙扫描并创建 lv_timer 轮询结果刷新设备列表
 * @note  内部先调用 bt_scan_ui_stop() 清理上次残留，可重复进入扫描页；
 *        实际扫描在后台 pthread 中执行（pthread ≈ 裸机里的独立任务）
 */
void bt_scan_ui_start(void);

/**
 * @brief 停止扫描：删除轮询定时器并终止后台扫描线程
 * @note  离开扫描页（screen_1）前必须调用，否则定时器会访问已删除的界面对象
 */
void bt_scan_ui_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_H_ */
