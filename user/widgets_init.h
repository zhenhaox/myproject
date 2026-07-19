/**
 * @file widgets_init.h
 * @brief FreeType 字体兜底头文件：把生成代码里的编译期字体宏桥接到运行时字体
 *
 * GUI Guider 生成的界面代码默认引用 LVGL 内置字库（&lv_font_xxx），
 * 内置字库没有中文字形。这里把每个 lv_font_xxx 定义为
 * "(*gui_guider_get_font(字号))"：生成代码里的 &lv_font_xxx 展开后变成
 * &(*指针)，正好取回 gui_guider_get_font() 返回的 FreeType 字体指针，
 * 从而不改一行生成代码就把字体整体换成运行时加载的中文字体。
 */

#ifndef WIDGETS_INIT_H
#define WIDGETS_INIT_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 字体兜底：NXP 生成的代码使用 &lv_font_XXX，这里用宏转成 FreeType 字体 */
/**
 * @brief 按字号取 FreeType 运行时字体（首次调用时完成全部字号初始化）
 * @param size 字号的像素高度，仅支持 font_sizes 表内的档位
 * @return 对应字号的字体指针；字号不支持或 FreeType 初始化失败时返回
 *         LVGL 内置 lv_font_montserrat_16 兜底（不含中文，仅保证不崩）
 * @note  只能在 GUI 线程调用（LVGL 非线程安全）
 */
const lv_font_t *gui_guider_get_font(int size);

/* 以下宏把生成代码中的字体引用重定向到 FreeType 运行时字体，宏名即原字库名 */
#define lv_font_Alatsi_Regular_35               (*gui_guider_get_font(35))  /**< 数字时钟用西文字体 35px */
#define lv_font_SourceHanSerifSC_Regular_52     (*gui_guider_get_font(52))  /**< 思源宋体 52px（大标题） */
#define lv_font_SourceHanSerifSC_Regular_34     (*gui_guider_get_font(34))  /**< 思源宋体 34px */
#define lv_font_SourceHanSerifSC_Regular_30     (*gui_guider_get_font(30))  /**< 思源宋体 30px */
#define lv_font_SourceHanSerifSC_Regular_26     (*gui_guider_get_font(26))  /**< 思源宋体 26px */
#define lv_font_SourceHanSerifSC_Regular_29     (*gui_guider_get_font(29))  /**< 思源宋体 29px */
#define lv_font_SourceHanSerifSC_Regular_24     (*gui_guider_get_font(24))  /**< 思源宋体 24px（列表正文） */

#ifdef __cplusplus
}
#endif

#endif /* WIDGETS_INIT_H */
