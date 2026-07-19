/*
 * widgets_init.h
 * FreeType 字体兜底头文件
 */

#ifndef WIDGETS_INIT_H
#define WIDGETS_INIT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 字体兜底：NXP 生成的代码使用 &lv_font_XXX，这里用宏转成 FreeType 字体 */
const lv_font_t *gui_guider_get_font(int size);

#define lv_font_Alatsi_Regular_35               (*gui_guider_get_font(35))
#define lv_font_SourceHanSerifSC_Regular_52     (*gui_guider_get_font(52))
#define lv_font_SourceHanSerifSC_Regular_34     (*gui_guider_get_font(34))
#define lv_font_SourceHanSerifSC_Regular_30     (*gui_guider_get_font(30))
#define lv_font_SourceHanSerifSC_Regular_26     (*gui_guider_get_font(26))
#define lv_font_SourceHanSerifSC_Regular_29     (*gui_guider_get_font(29))
#define lv_font_SourceHanSerifSC_Regular_24     (*gui_guider_get_font(24))

#ifdef __cplusplus
}
#endif

#endif /* WIDGETS_INIT_H */
