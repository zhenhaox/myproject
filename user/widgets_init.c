/*
 * widgets_init.c
 * FreeType 字体兜底实现；图片资源已迁移到 img_resources.c
 */

#include "widgets_init.h"
#include <stdio.h>
#include <string.h>

#ifndef GUI_GUIDER_FONT_FILE
#define GUI_GUIDER_FONT_FILE "/usr/share/fonts/source-han-sans-cn/SourceHanSansCN-Normal.otf"
#endif

/* 需要兜底的字体尺寸数量 */
#define FONT_SLOT_COUNT 7

static lv_ft_info_t font_slots[FONT_SLOT_COUNT];
static const int font_sizes[FONT_SLOT_COUNT] = { 24, 26, 29, 30, 34, 35, 52 };

const lv_font_t *gui_guider_get_font(int size)
{
    static int initialized = 0;

    if (!initialized) {
        for (int i = 0; i < FONT_SLOT_COUNT; i++) {
            font_slots[i].name = GUI_GUIDER_FONT_FILE;
            font_slots[i].weight = font_sizes[i];
            font_slots[i].style = FT_FONT_STYLE_NORMAL;
            font_slots[i].mem = NULL;
            if (!lv_ft_font_init(&font_slots[i])) {
                LV_LOG_USER("Failed to init freetype font size %d", font_sizes[i]);
                font_slots[i].font = NULL;
            }
        }
        initialized = 1;
    }

    for (int i = 0; i < FONT_SLOT_COUNT; i++) {
        if (font_sizes[i] == size) {
            if (font_slots[i].font != NULL) {
                return font_slots[i].font;
            }
            break;
        }
    }

    /* 兜底：返回 LVGL 内置字体 */
    return &lv_font_montserrat_16;
}

/* 图片资源已迁移到 img_resources.c，此处不再使用占位图 */
