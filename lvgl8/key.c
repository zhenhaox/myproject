/**
 * @file key.c
 * @brief 物理按键输入驱动（Linux evdev 接口），作为 LVGL 键盘(KEYPAD)输入设备的数据源。
 *        通过读取 /dev/input/eventX 节点拿到内核 input_event（/dev 节点 ≈ 外设寄存器的文件化接口），
 *        再把 Linux 键值映射成 LVGL 的 LV_KEY_xxx，供按键组(group)做焦点切换。
 * @note  本工程默认 USE_KEY=0，该文件不参与编译，仅作为预留的按键适配模板。
 */

/*********************
 *      INCLUDES
 *********************/
#include "key.h"
#if USE_KEY != 0

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static int key_fd;      /**< 按键设备节点(/dev/input/eventX)的文件描述符 */
static int key_button;  /**< 缓存最近一次按键状态：LV_INDEV_STATE_PR(按下) / LV_INDEV_STATE_REL(松开) */

static int key_val;     /**< 缓存最近一次映射后的 LVGL 键值(LV_KEY_xxx)，无新事件时保持上报 */
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Initialize the evdev interface
 * @brief  打开按键设备节点并配置为非阻塞模式
 * @return 0: 成功; -1: 打开设备节点失败（检查 KEY_NAME 与实际 event 编号是否对应）
 * @note   O_NONBLOCK 是必须的：LVGL 的 read_cb 由 lv_timer 周期轮询调用，
 *         若阻塞等待按键会卡死整个 GUI 线程。
 */
int key_init(void)
{
    key_fd = open(KEY_NAME, O_RDWR | O_NOCTTY | O_NDELAY);

    if (key_fd == -1)
    {
        perror("unable open evdev interface:");
        return -1;
    }

    fcntl(key_fd, F_SETFL, O_ASYNC | O_NONBLOCK);

    key_val = 0;
    key_button = LV_INDEV_STATE_REL;

    return 0;
}

/**
 * Get the current position and state of the evdev
 * @brief  LVGL 周期性回调：读取内核按键事件并翻译成 LVGL 键值
 * @param  drv  调用本回调的输入设备驱动对象（用于判断设备类型）
 * @param  data store the evdev data here（输出参数：回写键值与按下/松开状态）
 * @return false: because the points are not buffered, so no more data to be read
 * @note   事件循环只处理到第一个 EV_KEY 事件就返回，避免一次回调吞掉
 *         多个按键导致丢状态；无新事件时回放上一次缓存的键值/状态，
 *         否则 LVGL 会误以为按键被异常释放。
 */
void key_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    struct input_event in;  // 内核输入事件结构体：type(类型)/code(键码)/value(1按下 0松开 2长按重复)

    // 非阻塞读：把驱动缓冲里积压的事件尽量读完（≈ 中断标志轮询清标志）
    while (read(key_fd, &in, sizeof(struct input_event)) > 0)
    {
        if (in.type == EV_KEY)  // 只关心按键事件，忽略 EV_SYN 等同步事件
        {
            data->state = (in.value) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
            // 下面的 1/139/105/106 是内核 Linux 键码(input-event-codes.h)，
            // 不同按键驱动上报的 code 不同，换板子/换驱动时要重新对照修改
            switch (in.code)
            {
            case 1:
                data->key = LV_KEY_ESC;
                break;
            case 139:
                data->key = LV_KEY_ENTER;
                break;
                //case KEY_UP:
                //    data->key = LV_KEY_UP;
                //    break;
            case 105:
                data->key = LV_KEY_PREV;
                break;
            case 106:
                data->key = LV_KEY_NEXT;
                break;
                //case KEY_DOWN:
                //    data->key = LV_KEY_DOWN;
                //    break;
            default:
                data->key = 0;
                break;
            }
            key_val = data->key;      // 缓存本次键值与状态，供“无新事件”分支回放
            key_button = data->state;
            return ;                  // 每次回调只消化一个按键事件，其余留给下一周期
        }
    }

    if (drv->type == LV_INDEV_TYPE_KEYPAD)
    {
        /* No data retrieved */
        // 本轮没读到新事件：回放上次缓存值，保持 LVGL 侧的按键状态连续
        data->key = key_val;
        data->state = key_button;
        return ;
    }
}
#endif
