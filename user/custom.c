/*
 * custom.c
 */

#include "custom.h"
#include "gui_guider.h"
#include <stdio.h>
#include <time.h>

/* 外部时钟变量，由 setup_scr_screen.c 定义 */
extern int screen_clock_min_value;
extern int screen_clock_hour_value;
extern int screen_clock_sec_value;

/* 设置 lv_img 对象以 cover 模式铺满 w x h，按实际图片尺寸计算 uniform zoom */
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

    if (header.w == 0 || header.h == 0) {
        return;
    }

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

/* 在 parent 内创建一张 cover 模式铺满的底图，并移到最底层 */
lv_obj_t *lv_img_create_cover(lv_obj_t *parent, const char *path, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *img = lv_img_create(parent);
    lv_obj_set_pos(img, 0, 0);
    lv_img_set_src_cover(img, path, w, h);
    lv_obj_move_background(img);
    return img;
}

void screen_datetext_event_handler(lv_event_t *e)
{
    (void)e;
    /* 占位：日期文本点击事件 */
}

void screen_clock_timer(lv_timer_t *timer)
{
    (void)timer;

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
