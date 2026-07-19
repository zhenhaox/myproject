/*
 * gui_guider.h
 * NXP GUI Guider 导出代码的最小适配层
 */

#ifndef GUI_GUIDER_H_
#define GUI_GUIDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {
    /* screen */
    lv_obj_t *screen;
    lv_obj_t *screen_cont_1;
    lv_obj_t *screen_clock;
    lv_obj_t *screen_datetext;
    lv_obj_t *screen_disconnect_image;
    lv_obj_t *screen_adjust_btn;
    lv_obj_t *screen_adjust_btn_label;
    lv_obj_t *screen_printer_btn;
    lv_obj_t *screen_printer_btn_label;
    lv_obj_t *screen_printer_popup;
    lv_obj_t *screen_myprinter_label;
    lv_obj_t *screen_addprinter_btn;
    lv_obj_t *screen_addprinter_btn_label;
    lv_obj_t *screen_cancer_btn;
    lv_obj_t *screen_cancer_btn_label;
    lv_obj_t *screen_img_1;
    lv_obj_t *screen_img_2;

    /* screen_1 */
    lv_obj_t *screen_1;
    lv_obj_t *screen_1_cont_1;
    lv_obj_t *screen_1_scanbt_img;
    lv_obj_t *screen_1_cancer_btn;
    lv_obj_t *screen_1_cancer_btn_label;
    lv_obj_t *screen_1_brid_img;
    lv_obj_t *screen_1_cont_2;
    lv_obj_t *screen_1_printer_label;

    /* del flags used by ui_load_scr_animation */
    bool screen_del;
    bool screen_1_del;
} lv_ui;

extern lv_ui guider_ui;

void setup_scr_screen(lv_ui *ui);
void setup_scr_screen_1(lv_ui *ui);

void ui_load_scr_animation(lv_ui *ui,
                           lv_obj_t **new_scr,
                           bool *new_scr_del,
                           bool *old_scr_del,
                           void (*setup_fn)(lv_ui *),
                           lv_scr_load_anim_t anim_type,
                           uint32_t time,
                           uint32_t delay,
                           bool auto_del,
                           bool old_del);

#ifdef __cplusplus
}
#endif

#endif /* GUI_GUIDER_H_ */
