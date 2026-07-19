/**
 * @file gui_guider.c
 * @brief GUI Guider 适配层实现：全局 UI 实例与带删除标志维护的切屏函数
 *
 * 核心问题是"screen 什么时候删、什么时候重建"：LVGL 的
 * lv_scr_load_anim(auto_del=true) 会在动画结束后自动释放旧 screen，
 * 但被切走的 screen 指针还留在 guider_ui 里。这里用 xxx_del 标志记账，
 * 下次切回时按需重建，避免重复创建占内存、也避免使用已释放的指针。
 */

#include "gui_guider.h"

lv_ui guider_ui;  /**< 全局唯一 UI 实例（全部控件指针集中于此） */

/**
 * @brief 带动画（可选）切换到新 screen，并维护双方的删除标志
 * @param ui          全局 UI 结构体指针（未使用，保留生成代码签名）
 * @param new_scr     新 screen 指针的地址；为空或已删则先调 setup_fn 重建
 * @param new_scr_del 新 screen 删除标志的地址，重建后清 false；可为 NULL
 * @param old_scr_del 旧 screen 删除标志的地址，auto_del 为 true 时置 true；可为 NULL
 * @param setup_fn    新 screen 的构建函数（setup_scr_xxx）
 * @param anim_type   切换动画类型（LV_SCR_LOAD_ANIM_xxx）
 * @param time        动画时长（ms）
 * @param delay       动画延迟（ms）
 * @param auto_del    true = LVGL 动画结束后自动删除旧 screen
 * @param old_del     生成代码保留参数，未使用
 * @note  只在新 screen 不存在或被删过时才重建：未删过的 screen 直接复用，
 *        其上控件的旧状态会保留（调用方如需刷新需自行处理）
 */
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
