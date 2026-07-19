/**
 * @file gui_guider.h
 * @brief NXP GUI Guider 导出代码的最小适配层：全局 UI 结构体与切屏接口
 *
 * lv_ui 汇总了所有 screen 及其控件的指针（命名规则：<屏幕名>_<控件名>），
 * 全局实例 guider_ui 贯穿整个程序。xxx_del 标志记录"该 screen 是否已被
 * LVGL 自动删除"，是切屏时决定要不要重建 screen 的依据。
 */

#ifndef GUI_GUIDER_H_
#define GUI_GUIDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief 全局 UI 结构体：集中持有所有 screen 与控件的指针
 * @note  控件指针在 setup_scr_xxx() 创建界面时赋值；screen 被删除后
 *        对应指针即失效，访问前需结合 xxx_del 标志或判空
 */
typedef struct {
    /* screen（主界面：时钟、打印机入口、弹窗） */
    lv_obj_t *screen;                     /**< 主界面屏幕根对象 */
    lv_obj_t *screen_cont_1;              /**< 主界面容器 */
    lv_obj_t *screen_clock;               /**< 时钟文本 label（HH:MM:SS） */
    lv_obj_t *screen_datetext;            /**< 日期文本 label */
    lv_obj_t *screen_disconnect_image;    /**< "断开连接"状态图片，点击弹窗 */
    lv_obj_t *screen_adjust_btn;          /**< 校准按钮 */
    lv_obj_t *screen_adjust_btn_label;    /**< 校准按钮文字 */
    lv_obj_t *screen_printer_btn;         /**< 打印机按钮 */
    lv_obj_t *screen_printer_btn_label;   /**< 打印机按钮文字 */
    lv_obj_t *screen_printer_popup;       /**< 打印机选择弹窗（默认隐藏） */
    lv_obj_t *screen_myprinter_label;     /**< 弹窗标题"我的打印机" */
    lv_obj_t *screen_addprinter_btn;      /**< 弹窗内"添加打印机"按钮 */
    lv_obj_t *screen_addprinter_btn_label;/**< "添加打印机"按钮文字 */
    lv_obj_t *screen_cancer_btn;          /**< 弹窗"取消"按钮（cancer 为生成器拼写，实为 cancel） */
    lv_obj_t *screen_cancer_btn_label;    /**< "取消"按钮文字 */
    lv_obj_t *screen_img_1;               /**< 装饰图片 1 */
    lv_obj_t *screen_img_2;               /**< 装饰图片 2 */

    /* screen_1（蓝牙扫描页：雷达动画 + 设备列表） */
    lv_obj_t *screen_1;                   /**< 蓝牙扫描页屏幕根对象 */
    lv_obj_t *screen_1_cont_1;            /**< 扫描页上半部容器（雷达图区） */
    lv_obj_t *screen_1_scanbt_img;        /**< 扫描雷达图片（旋转动画对象） */
    lv_obj_t *screen_1_cancer_btn;        /**< 扫描页"取消"按钮（返回主界面） */
    lv_obj_t *screen_1_cancer_btn_label;  /**< "取消"按钮文字 */
    lv_obj_t *screen_1_brid_img;          /**< 装饰图片 */
    lv_obj_t *screen_1_cont_2;            /**< 扫描页下半部容器（列表区） */
    lv_obj_t *screen_1_printer_label;     /**< "选择打印机"提示文字 */
    lv_obj_t *screen_1_scan_status_label; /**< 扫描状态文字（正在搜索/完成/出错） */
    lv_obj_t *screen_1_device_list;       /**< 扫描到的蓝牙设备列表 */

    /* del flags used by ui_load_scr_animation */
    bool screen_del;                      /**< true 表示 screen 已被 LVGL 自动删除，需重建 */
    bool screen_1_del;                    /**< true 表示 screen_1 已被 LVGL 自动删除，需重建 */
} lv_ui;

extern lv_ui guider_ui;  /**< 全局唯一 UI 实例，定义在 gui_guider.c */

/**
 * @brief 创建主界面 screen 的全部控件（GUI Guider 生成，实现在 setup_scr_screen.c）
 * @param ui 全局 UI 结构体指针
 */
void setup_scr_screen(lv_ui *ui);

/**
 * @brief 创建蓝牙扫描页 screen_1 的全部控件（实现在 setup_scr_screen_1.c）
 * @param ui 全局 UI 结构体指针
 */
void setup_scr_screen_1(lv_ui *ui);

/**
 * @brief 带动画（可选）切换到新 screen，并维护双方的删除标志
 * @param ui          全局 UI 结构体指针（当前未使用，保留生成代码签名）
 * @param new_scr     新 screen 指针的地址；为空或已删则先调 setup_fn 重建
 * @param new_scr_del 新 screen 删除标志的地址，重建后清 false；可为 NULL
 * @param old_scr_del 旧 screen 删除标志的地址，auto_del 为 true 时置 true；可为 NULL
 * @param setup_fn    新 screen 的构建函数（setup_scr_xxx）
 * @param anim_type   切换动画类型（LV_SCR_LOAD_ANIM_xxx）
 * @param time        动画时长（ms）
 * @param delay       动画延迟（ms）
 * @param auto_del    true = LVGL 动画结束后自动删除旧 screen
 * @param old_del     生成代码保留参数，当前未使用
 * @note  该函数只负责"按需重建 + 加载 + 记账"，动画与旧屏释放由 LVGL 完成
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
                           bool old_del);

#ifdef __cplusplus
}
#endif

#endif /* GUI_GUIDER_H_ */
