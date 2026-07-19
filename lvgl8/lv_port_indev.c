/**
 * @file lv_port_indev.c
 * @brief LVGL 输入设备移植层：向 LVGL 注册触摸/按键等输入设备，
 *        并维护"按键组(group)"链表——group 决定按键焦点落在哪个界面的控件上，
 *        多用于有物理按键的界面导航；本工程以电容触摸为主要输入。
 */

/* Copy this file as "lv_port_indev.c" and set this value to "1" to enable conten */

#include <stdlib.h>
#include <lvgl.h>
#include <lvgl/lv_conf.h>
#include <lv_drivers/sdl/sdl_gpu.h>

#include "lv_port_indev.h"
#include "key.h"

// 按键组链表节点：每创建一个界面可建一个 group，链表头插法管理，
// 销毁界面时从链表摘除并把按键焦点交还给上一个 group
typedef struct _GROUP_NODE
{
    struct _GROUP_NODE *next;   /**< 指向下一个节点（按创建顺序倒序串联） */
    lv_group_t *group;          /**< LVGL 按键组对象 */
} GROUP_NODE;

static int rot_indev;                    /**< 缓存输入设备的坐标旋转角度（配合屏幕旋转） */
static lv_indev_t *indev_touchpad;       /**< 已注册的触摸输入设备(evdev) */
static lv_indev_t *indev_key;            /**< 已注册的物理按键输入设备（USE_KEY=1 时有效） */
static lv_indev_t *indev_sdl;            /**< PC 模拟用的 SDL 鼠标输入设备 */

GROUP_NODE *group_list = NULL;           /**< 按键组链表头（最新创建的 group 在表头） */

#ifdef USE_SENSOR
// 传感器（光感/接近感应等）驱动对象：故意不注册进 LVGL，
// 只把 read_cb 暴露给应用层自行调用（见 lv_port_indev_init 末尾注释）
static lv_indev_drv_t lsensor_drv;
static lv_indev_drv_t psensor_drv;

/**
 * @brief  获取光传感器驱动对象（未注册到 LVGL，由应用层直接驱动）
 * @return 光传感器 lv_indev_drv_t 指针
 */
lv_indev_drv_t *lv_port_indev_get_lsensor_drv(void)
{
    return &lsensor_drv;
}

/**
 * @brief  获取接近传感器驱动对象（未注册到 LVGL，由应用层直接驱动）
 * @return 接近传感器 lv_indev_drv_t 指针
 */
lv_indev_drv_t *lv_port_indev_get_psensor_drv(void)
{
    return &psensor_drv;
}
#endif

/**
 * @brief  创建一个按键组(group)并挂到管理链表头
 * @return 新建的 lv_group_t 指针
 * @note   创建后立刻把按键输入设备绑定到该组并设为默认组，
 *         即"新界面的控件立即获得按键焦点"；调用方（界面销毁时）
 *         必须配对调用 lv_port_indev_group_destroy()，否则链表内存泄漏。
 */
lv_group_t *lv_port_indev_group_create(void)
{
    struct _GROUP_NODE *group_node = NULL;
    lv_group_t *group = lv_group_create();

    lv_indev_set_group(indev_key, group);
    lv_group_set_default(group);

    group_node = (struct _GROUP_NODE *)malloc(sizeof(struct _GROUP_NODE));
    group_node->group = group;
    group_node->next = NULL;
    // 头插法：最新创建的 group 总在表头，方便销毁时快速回退焦点
    if (group_list)
    {
        group_node->next = group_list;
        group_list = group_node;
    }
    else
    {
        group_list = group_node;
    }

    return group;
}

/**
 * @brief  销毁按键组：从管理链表摘除节点，并把按键焦点移交给剩余链表头的 group
 * @param  group 待销毁的 group（通常在被销毁界面关联的那个）
 * @note   若销毁的是表头 group，焦点自动回退到新的表头（即上一个界面）；
 *         若链表已空，则按键设备解绑、默认组清空，按键暂时失效。
 *         注意：仅当 group_list 非空时才会真正执行 lv_group_del()。
 */
void lv_port_indev_group_destroy(lv_group_t *group)
{
    if (group_list)
    {
        struct _GROUP_NODE *group_node = NULL;
        group_node = group_list;
        if (group_list->group == group)
        {
            // 待销毁的正是表头：摘除后把按键焦点回退给新表头（上一个界面）
            group_list = group_list->next;
            if (group_list)
            {
                lv_indev_set_group(indev_key, group_list->group);
                lv_group_set_default(group_list->group);
            }
            else
            {
                lv_indev_set_group(indev_key, NULL);
                lv_group_set_default(NULL);
            }
            free(group_node);
        }
        else
        {
            while (group_node->next)
            {
                struct _GROUP_NODE *group_node_next = group_node->next;
                if (group_node_next->group == group)
                {
                    group_node->next = group_node_next->next;
                    free(group_node_next);
                    break;
                }
                group_node = group_node->next;
            }
        }
        lv_group_del(group);
    }
}

/**
 * @brief  初始化并注册全部输入设备（触摸/按键/SDL 鼠标，按编译开关裁剪）
 * @param  rot 输入坐标旋转角度（0/90/180/270），须与屏幕旋转一致，
 *         否则触摸坐标与画面不对应（点这里、响应在那里）
 * @note   LVGL 输入设备模型 ≈ MCU 里"注册中断回调"：这里注册的 read_cb
 *         由 LVGL 内核周期轮询调用，回调里必须快速返回，禁止阻塞。
 */
void lv_port_indev_init(int rot)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    // 驱动对象必须 static：注册后 LVGL 内核长期持有指针，栈变量会悬空
    static lv_indev_drv_t indev_drv;
    static lv_indev_drv_t key_drv;
    static lv_indev_drv_t sdl_drv;
    lv_disp_t *disp;

    rot_indev = rot;

    /*------------------
     * Touchpad
     * -----------------*/
#if USE_EVDEV != 0 || USE_BSD_EVDEV
    /*Initialize your touchpad if you have*/
    // 本工程主路径：打开电容触摸对应的 /dev/input/eventX（见 evdev.c），
    // 初始化成功才注册进 LVGL；失败则静默跳过，界面仍可用按键/SDL 操作
    disp = lv_disp_get_default();
    if (evdev_init(disp->driver, rot) == 0)
    {
        /*Register a touchpad input device*/
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;   // 指针类设备：上报 (x,y) 坐标 + 按下状态
        indev_drv.read_cb = evdev_read;
        indev_touchpad = lv_indev_drv_register(&indev_drv);
    }
#endif
#if USE_KEY
    // 物理按键：注册为 KEYPAD 设备并创建首个按键组，
    // 之后界面才能用 LV_KEY_PREV/NEXT/ENTER 做焦点导航
    if (key_init() == 0)
    {
        lv_indev_drv_init(&key_drv);
        key_drv.type = LV_INDEV_TYPE_KEYPAD;
        key_drv.read_cb = key_read;
        indev_key = lv_indev_drv_register(&key_drv);
        lv_port_indev_group_create();
    }
#endif

#if USE_SDL_GPU
    // PC 模拟路径：SDL 窗口内的鼠标当作触摸用
    lv_indev_drv_init(&sdl_drv);
    sdl_drv.type = LV_INDEV_TYPE_POINTER;
    sdl_drv.read_cb = sdl_mouse_read;
    indev_sdl = lv_indev_drv_register(&sdl_drv);
#endif

#if USE_SENSOR
    lv_indev_drv_init(&lsensor_drv);
    if (evdev_init_lsensor() >= 0)
    {
        lsensor_drv.type = LV_INDEV_TYPE_NONE;
        lsensor_drv.user_data = evdev_get_lsensor();
        lsensor_drv.read_cb = evdev_sensor_read;
        /* DO NOT register the drv to lvgl, just handled by applications */
        // 故意不调用 lv_indev_drv_register()：传感器数据不走 LVGL 焦点/事件体系，
        // 应用层通过 lv_port_indev_get_lsensor_drv() 拿到驱动自行周期读取
    }

    lv_indev_drv_init(&psensor_drv);
    if (evdev_init_psensor() >= 0)
    {
        psensor_drv.type = LV_INDEV_TYPE_NONE;
        psensor_drv.user_data = evdev_get_psensor();
        psensor_drv.read_cb = evdev_sensor_read;
        /* DO NOT register the drv to lvgl, just handled by applications */
    }
#endif
}

