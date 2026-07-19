/**
 * @file custom.c
 * @brief 用户自定义逻辑实现：时钟刷新、图片 cover 填充、蓝牙扫描页 UI 胶水
 *
 * 蓝牙扫描是典型的"工作线程 + GUI 轮询"模型：bt_scan 模块在后台
 * pthread 里跑 BlueZ D-Bus 扫描，本文件用 lv_timer 每 500ms 把结果
 * 增量刷进列表。LVGL 非线程安全，所有 LVGL API 只能由 GUI 线程
 * （lv_timer 回调所在线程）调用，工作线程严禁直接碰控件。
 */

#include "custom.h"
#include "gui_guider.h"
#include "widgets_init.h"
#include "bt_scan.h"
#include <stdio.h>
#include <time.h>

/* 外部时钟变量，由 setup_scr_screen.c 定义 */
extern int screen_clock_min_value;
extern int screen_clock_hour_value;
extern int screen_clock_sec_value;

/**
 * @brief 让图片以 cover 模式（等比放大、裁掉溢出部分）铺满 w x h 区域
 * @param img  目标 lv_img 对象
 * @param path 图片路径（LVGL 文件系统路径）
 * @param w    目标宽度（像素）
 * @param h    目标高度（像素）
 * @note  LVGL 的 zoom 为 1/256 定点：256=原尺寸，512=2 倍。
 *        解码失败时退化为直接设置尺寸，保证界面至少能显示出内容。
 */
void lv_img_set_src_cover(lv_obj_t *img, const char *path, lv_coord_t w, lv_coord_t h)
{
    lv_img_header_t header;
    if (lv_img_decoder_get_info(path, &header) != LV_RES_OK) {
        LV_LOG_USER("Failed to get image info: %s !!!!!!", path);
        lv_img_set_src(img, path);
        lv_obj_set_size(img, w, h);
        return;
    }

    lv_img_set_src(img, path);
    lv_obj_set_size(img, w, h);

    /* 防止异常图片头导致除零 */
    if (header.w == 0 || header.h == 0) {
        return;
    }

    /* cover 模式取两个方向中较大的缩放比，保证短边也铺满（多余部分被裁掉） */
    int32_t zoom_x = ((int32_t)w * 256) / header.w;
    int32_t zoom_y = ((int32_t)h * 256) / header.h;
    int32_t zoom = (zoom_x > zoom_y) ? zoom_x : zoom_y;
    if (zoom < 1) {
        zoom = 1;
    }
    if (zoom > 512) {
        zoom = 512; /* LVGL zoom 上限通常是 512 */
    }
    lv_img_set_zoom(img, zoom);
}

/**
 * @brief 在 parent 内新建一张 cover 模式铺满的图片，并压到最底层当背景
 * @param parent 父对象（通常是某个 screen）
 * @param path   图片路径
 * @param w      目标宽度（像素）
 * @param h      目标高度（像素）
 * @return 创建成功的 lv_img 对象指针
 */
lv_obj_t *lv_img_create_cover(lv_obj_t *parent, const char *path, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *img = lv_img_create(parent);
    lv_obj_set_pos(img, 0, 0);
    lv_img_set_src_cover(img, path, w, h);
    lv_obj_move_background(img);  /* 压到兄弟对象最底层，避免挡住上面的控件 */
    return img;
}

/**
 * @brief 日期文本的点击事件回调（占位，暂未实现功能）
 * @param e LVGL 事件对象，当前未使用
 */
void screen_datetext_event_handler(lv_event_t *e)
{
    (void)e;
    /* 占位：日期文本点击事件 */
}

/**
 * @brief 主界面时钟的秒级刷新回调，由 lv_timer 每秒触发一次
 * @param timer 触发本回调的定时器对象，当前未使用
 * @note  纯软件计时：直接自增时/分/秒全局变量再刷新 label，不读 RTC。
 *        类似 MCU 定时器中断里的软件时钟；时/分/秒变量定义在
 *        setup_scr_screen.c，切换屏幕前必须把 guider_ui.screen_clock
 *        置 NULL，否则 screen 删除后这里会写野指针（见 events_init.c）。
 */
void screen_clock_timer(lv_timer_t *timer)
{
    (void)timer;

    /* 软件进位链：秒满 60 进分，分满 60 进时，时满 24 归零 */
    screen_clock_sec_value++;
    if (screen_clock_sec_value >= 60) {
        screen_clock_sec_value = 0;
        screen_clock_min_value++;
        if (screen_clock_min_value >= 60) {
            screen_clock_min_value = 0;
            screen_clock_hour_value++;
            if (screen_clock_hour_value >= 24) {
                screen_clock_hour_value = 0;
            }
        }
    }

    if (guider_ui.screen_clock) {
        lv_label_set_text_fmt(guider_ui.screen_clock, "%02d:%02d:%02d",
                              screen_clock_hour_value,
                              screen_clock_min_value,
                              screen_clock_sec_value);
    }
}

/* 蓝牙扫描页 UI 胶水：扫描在线程中跑，这里用 lv_timer 轮询结果刷新列表 */

static lv_timer_t *g_bt_poll_timer = NULL;  /**< 扫描结果轮询定时器（扫描页生命周期内唯一） */
static int g_bt_shown_count = 0;            /**< 已添加到列表的设备数，用于增量刷新 */

/**
 * @brief 结束本次扫描的 UI 侧收尾：停雷达动画、删轮询定时器
 * @param timer 待删除的轮询定时器（即 g_bt_poll_timer 本身）
 * @note  扫描完成、出错或界面对象已销毁时统一走这里收尾；
 *        删除后把 g_bt_poll_timer 置 NULL，避免重复删除
 */
static void bt_scan_poll_finish(lv_timer_t *timer)
{
    if (guider_ui.screen_1_scanbt_img) {
        lv_anim_del(guider_ui.screen_1_scanbt_img, (lv_anim_exec_xcb_t)lv_img_set_angle);
    }
    lv_timer_del(timer);
    g_bt_poll_timer = NULL;
}

/**
 * @brief 扫描结果轮询回调（500ms 周期）：把新发现的设备增量刷进列表
 * @param timer 轮询定时器对象
 * @note  本回调运行在 GUI 线程，可以安全调用 LVGL API；
 *        设备数据由 bt_scan 工作线程写入、经 bt_scan_get_devices()
 *        加锁拷贝出来（线程间只传数据，不传 LVGL 对象）。
 *        扫描结束或出错后会自我删除（只触发一次的"软中断"式收尾）。
 */
static void bt_scan_poll_timer(lv_timer_t *timer)
{
    /* 防御：扫描页被销毁后定时器本应已删除，此处仅为双保险 */
    if (guider_ui.screen_1_device_list == NULL ||
        guider_ui.screen_1_scan_status_label == NULL) {
        bt_scan_poll_finish(timer);
        return;
    }

    bt_device_t devs[BT_SCAN_MAX_DEVICES];
    int n = bt_scan_get_devices(devs, BT_SCAN_MAX_DEVICES);

    /* 只增量追加新发现的设备，避免列表重排闪烁 */
    for (int i = g_bt_shown_count; i < n; i++) {
        char text[96];
        snprintf(text, sizeof(text), "%s (%s)", devs[i].name, devs[i].addr);
        lv_obj_t *btn = lv_list_add_btn(guider_ui.screen_1_device_list, NULL, text);
        /* 主题默认字体不含中文，设备名可能为中文，需显式指定中文字体 */
        lv_obj_t *label = lv_obj_get_child(btn, -1);
        if (label) {
            lv_obj_set_style_text_font(label, &lv_font_SourceHanSerifSC_Regular_24, LV_STATE_DEFAULT);
        }
    }
    g_bt_shown_count = n;

    const char *err = bt_scan_get_error();
    if (err != NULL) {
        lv_label_set_text_fmt(guider_ui.screen_1_scan_status_label, "蓝牙不可用: %s", err);
        bt_scan_poll_finish(timer);
        return;
    }

    if (bt_scan_is_running()) {
        lv_label_set_text_fmt(guider_ui.screen_1_scan_status_label, "正在搜索... (%d)", n);
    } else {
        lv_label_set_text_fmt(guider_ui.screen_1_scan_status_label, "搜索完成，共发现 %d 台设备", n);
        bt_scan_poll_finish(timer);
    }
}

/**
 * @brief 启动蓝牙扫描：开后台扫描线程 + 建 500ms 轮询定时器
 * @note  进入扫描页（screen_1）时调用。先 stop 一次是为了清理上次
 *        可能残留的定时器/线程，保证重复进入页面时状态干净；
 *        g_bt_shown_count 清零，让新列表从头开始增量刷新。
 */
void bt_scan_ui_start(void)
{
    bt_scan_ui_stop();  /* 防御：清理上次可能残留的定时器与线程 */
    g_bt_shown_count = 0;
    bt_scan_start();
    g_bt_poll_timer = lv_timer_create(bt_scan_poll_timer, 500, NULL);
}

/**
 * @brief 停止蓝牙扫描：删轮询定时器 + 终止后台扫描线程
 * @note  离开扫描页前必须调用（见 events_init.c 的 load_screen_timer_cb），
 *        否则 screen_1 删除后定时器回调会访问野指针。
 *        顺序上先删定时器再停线程：定时器删了就不再有人读列表控件。
 */
void bt_scan_ui_stop(void)
{
    if (g_bt_poll_timer) {
        lv_timer_del(g_bt_poll_timer);
        g_bt_poll_timer = NULL;
    }
    bt_scan_stop();
}
