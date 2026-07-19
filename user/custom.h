/*
 * custom.h
 */

#ifndef CUSTOM_H_
#define CUSTOM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void screen_datetext_event_handler(lv_event_t *e);
void screen_clock_timer(lv_timer_t *timer);

/* 图片自适应填充辅助函数 */
void lv_img_set_src_cover(lv_obj_t *img, const char *path, lv_coord_t w, lv_coord_t h);
lv_obj_t *lv_img_create_cover(lv_obj_t *parent, const char *path, lv_coord_t w, lv_coord_t h);

#ifdef __cplusplus
}
#endif

#endif /* CUSTOM_H_ */
