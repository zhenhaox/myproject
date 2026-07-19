/**
 * @file ui_scaler.h
 * @brief UI 尺寸缩放器接口头文件
 *
 * GUI Guider 生成的界面坐标/尺寸是按设计参考分辨率（本项目的 720x1280）写死的。
 * 本模块提供统一的缩放换算：界面初始化时把设计值经 ui_scaler_calc() 换算成
 * 实际屏幕分辨率下的像素值，从而同一套 UI 代码可适配不同分辨率的屏幕。
 * 接口以 void * 句柄形式暴露，内部结构体对调用方不可见（相当于 C 语言版的“私有成员”）。
 */
#ifndef __UI_SCALER_H__
#define __UI_SCALER_H__

/**
 * @brief 创建 UI 缩放器实例
 * @param width  实际屏幕宽度（像素）
 * @param height 实际屏幕高度（像素）
 * @return 缩放器句柄（内部为 ui_scaler 结构体）；内存不足返回 NULL
 * @note 内部使用 malloc 分配，不再使用时必须配对调用 ui_scaler_del() 释放。
 */
void *ui_scaler_new(int width, int height);

/**
 * @brief 设置横向/纵向缩放比的混合权重
 * @param scaler 缩放器句柄
 * @param rate   纵向占比，范围 [0,1]：0 表示完全按横向比例缩放，
 *               1 表示完全按纵向比例缩放，默认 0.5（横纵各占一半）
 * @note 设置后会用已保存的参考分辨率立即重算缩放比，
 *       因此必须先调用过 ui_scaler_set_refer_size() 才有意义。
 */
void ui_scaler_set_match_rate(void *scaler, float rate);

/**
 * @brief 设置 UI 设计时的参考分辨率并计算缩放比
 * @param scaler 缩放器句柄
 * @param width  设计参考宽度（像素，如 720）
 * @param height 设计参考高度（像素，如 1280）
 * @note 缩放比 = 横向比 * (1 - match_rate) + 纵向比 * match_rate，
 *       会打印一条调试日志到串口/终端。
 */
void ui_scaler_set_refer_size(void *scaler, int width, int height);

/**
 * @brief 把设计分辨率下的尺寸值换算为实际屏幕像素值
 * @param scaler 缩放器句柄；传 NULL 时原样返回 value（不缩放）
 * @param value  设计分辨率下的坐标或尺寸（像素）
 * @return 缩放后的像素值（int 截断取整）
 */
int ui_scaler_calc(void *scaler, int value);

/**
 * @brief 销毁缩放器实例
 * @param scaler 缩放器句柄，可为 NULL
 * @note 释放后句柄不可再使用。
 */
void ui_scaler_del(void *scaler);

#endif

