/**
 * @file evdev.h
 * @brief Linux evdev 输入设备（触摸屏/鼠标/键盘）接口头文件。
 *        声明 LVGL 输入驱动对接 evdev 所需的初始化、重设设备节点、
 *        读取坐标等接口。evdev 是 Linux 输入子系统的用户态接口，
 *        /dev/input/eventX 节点可类比为 MCU 里“触摸外设寄存器”的文件化入口，
 *        读它就能拿到内核驱动上报的坐标/按键事件。
 */

#ifndef EVDEV_H
#define EVDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------
 * Mouse or touchpad as evdev interface (for Linux based systems)
 *------------------------------------------------*/
/* 功能开关：通常在 lv_conf.h 或编译选项里定义，这里给默认值兜底 */
#ifndef USE_EVDEV
#  define USE_EVDEV           0    /* 是否启用 Linux evdev 触摸/鼠标输入 */
#endif

#ifndef USE_BSD_EVDEV
#  define USE_BSD_EVDEV       0    /* 是否启用 BSD 系统的 evdev 兼容层 */
#endif

#ifndef USE_SENSOR
#  define USE_SENSOR         0     /* 是否启用接近/光传感器（本板默认关闭） */
#endif

#ifndef USE_PSENSOR
#  define USE_PSENSOR         0    /* 是否启用接近传感器 */
#endif

#if USE_EVDEV || USE_BSD_EVDEV
#  undef EVDEV_NAME
#  define EVDEV_NAME   "/dev/input/event2"        /*You can use the "evtest" Linux tool to get the list of devices and test them*/
/* 触摸设备节点，编号随内核枚举顺序变化；实际运行时会由 evdev_get_tp_event() 自动探测覆盖 */
#  define EVDEV_SWAP_AXES         0               /*Swap the x and y axes of the touchscreen*/

/* 触摸原始坐标默认范围，与 720x1280 竖屏一致；打开设备后会用驱动上报的实际范围覆盖 */
#  define DEFAULT_EVDEV_HOR_MIN   0
#  define DEFAULT_EVDEV_HOR_MAX   720
#  define DEFAULT_EVDEV_VER_MIN   0
#  define DEFAULT_EVDEV_VER_MAX   1280
#endif  /*USE_EVDEV*/

#if USE_EVDEV || USE_BSD_EVDEV

#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize the evdev
 * 初始化 evdev 触摸输入：优先用环境变量 LV_EVENT_NAME 指定的节点，
 * 否则自动探测触摸屏设备，然后打开设备并启动后台采集线程。
 * @param drv  LVGL 显示驱动指针，用于取屏幕分辨率做坐标映射/裁剪
 * @param rot  触摸坐标旋转角度（0/90/180/270，与显示方向对齐）
 * @return 0 成功；-1 失败（未找到触摸设备或打开失败）
 */
int evdev_init(lv_disp_drv_t *drv, int rot);
/**
 * reconfigure the device file for evdev
 * 重新指定（或首次打开）evdev 设备节点：关闭旧句柄、读取设备上报的
 * 坐标范围、决定校准/裁剪策略，并创建采集线程与互斥锁。
 * @param drv      LVGL 显示驱动指针（取分辨率用）
 * @param dev_name set the evdev device filename（如 "/dev/input/event2"）
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 * @note 实际返回值为 0 成功 / -1 失败（旧注释的 true/false 描述不准确）
 */
int evdev_set_file(lv_disp_drv_t *drv, char *dev_name);
/**
 * Get the current position and state of the evdev
 * LVGL 输入设备的 read_cb 回调：LVGL 线程周期调用它取触摸坐标和按下状态，
 * 内部完成旋转、校准映射、裁剪和边界钳位。
 * @param drv  LVGL 输入设备驱动指针
 * @param data store the evdev data here（输出：坐标 point 与状态 state）
 * @return false: because the points are not buffered, so no more data to be read
 * @note 该函数与后台 evdev_thread 通过互斥锁交换数据，持锁时间很短
 */
void evdev_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

#if USE_SENSOR
int evdev_init_psensor(void);

void *evdev_get_psensor(void);

int evdev_init_lsensor(void);

void *evdev_get_lsensor(void);

void evdev_sensor_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
#endif

/**********************
 *      MACROS
 **********************/

#endif /* USE_EVDEV */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EVDEV_H */
