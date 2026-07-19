/**
 * @file ui_scaler.c
 * @brief UI 尺寸缩放器实现
 *
 * 把 GUI Guider 按设计参考分辨率（720x1280）生成的坐标/尺寸，
 * 统一换算为实际屏幕分辨率下的像素值，使同一套 UI 代码适配不同屏幕。
 * 结构体 ui_scaler 只在本文件内定义，对外以 void * 句柄操作，
 * 类似面向对象里的“私有成员”，调用方无法也不应直接访问字段。
 */
#include <stdio.h>
#include <stdlib.h>

/* 缩放器内部状态，字段含义见下方注释；对外隐藏，只允许通过 ui_scaler_xxx() 接口访问 */
typedef struct
{
    int width;          /**< 实际屏幕宽度（像素） */
    int height;         /**< 实际屏幕高度（像素） */
    int refer_width;    /**< UI 设计参考宽度（像素） */
    int refer_height;   /**< UI 设计参考高度（像素） */
    float scale_rate;   /**< 综合缩放比：横/纵向比例按 match_rate 加权混合的结果 */
    float match_rate;   /**< 纵向比例所占权重 [0,1]，默认 0.5 表示横纵各取一半 */
} ui_scaler;

void *ui_scaler_new(int width, int height)
{
    ui_scaler *scaler;

    /* 堆上分配实例（Linux 下 malloc ≈ 裸机的静态内存池申请，但失败会返回 NULL，必须检查） */
    scaler = malloc(sizeof(ui_scaler));
    if (!scaler) return NULL;

    scaler->width = width;
    scaler->height = height;
    scaler->scale_rate = 1.0f;   /* 未设置参考分辨率前保持 1:1，calc 原样返回 */
    scaler->match_rate = 0.5f;

    return scaler;
}

void ui_scaler_set_refer_size(void *scaler, int width, int height)
{
    ui_scaler *_scaler = (ui_scaler *)scaler;
    float rate_hor, rate_ver;

    /* 句柄为空直接返回，避免空指针段错误（Linux 下踩空指针进程直接被内核杀掉） */
    if (!scaler) return;

    /* 分别算横向、纵向缩放比，再按 match_rate 加权混合：
     * 防止屏幕宽高比与设计稿不一致时，单用某一方向比例导致另一方向溢出或留白过大 */
    rate_hor = (float)width / _scaler->width;
    rate_ver = (float)height / _scaler->height;

    _scaler->refer_width = width;
    _scaler->refer_height = height;
    _scaler->scale_rate = rate_hor * (1.0f - _scaler->match_rate) +
                          rate_ver * _scaler->match_rate;
    /* 调试输出：串口/终端确认缩放比是否符合预期 */
    printf("%d %d %f\n", width, height, _scaler->scale_rate);
}

void ui_scaler_set_match_rate(void *scaler, float rate)
{
    ui_scaler *_scaler = (ui_scaler *)scaler;

    if (!scaler) return;

    _scaler->match_rate = rate;
    /* 权重变了，用上次保存的参考分辨率重算缩放比，使新权重立即生效 */
    ui_scaler_set_refer_size(scaler, _scaler->refer_width,
                             _scaler->refer_height);
}

int ui_scaler_calc(void *scaler, int value)
{
    ui_scaler *_scaler = (ui_scaler *)scaler;

    /* 未创建缩放器时原样返回，保证上层代码不缩放也能正常运行 */
    if (!scaler) return value;

    /* int 强转为截断取整，坐标误差小于 1 像素，UI 上可忽略 */
    return (int)(value * _scaler->scale_rate);
}

void ui_scaler_del(void *scaler)
{
    /* free(NULL) 是安全的，这里无需判空 */
    free(scaler);
}

