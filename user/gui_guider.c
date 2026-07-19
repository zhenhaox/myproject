/*
 * gui_guider.c
 */

#include "gui_guider.h"

lv_ui guider_ui;

void ui_load_scr_animation(lv_ui *ui,
                           lv_obj_t **new_scr,
                           bool *new_scr_del,
                           bool *old_scr_del,
                           void (*setup_fn)(lv_ui *),
                           lv_scr_load_anim_t anim_type,
                           uint32_t time,
                           uint32_t delay,
                           bool auto_del,
                           bool old_del)
{
    (void)ui;
    (void)old_del;

    /* 如果新屏幕为空或已被自动删除，则重新创建 */
    if (*new_scr == NULL || (new_scr_del && *new_scr_del)) {
        setup_fn(&guider_ui);
        if (new_scr_del) {
            *new_scr_del = false;
        }
    }

    lv_scr_load_anim(*new_scr, anim_type, time, delay, auto_del);

    /* 标记旧屏幕已被自动删除 */
    if (auto_del && old_scr_del) {
        *old_scr_del = true;
    }
}
