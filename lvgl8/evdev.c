/**
 * @file evdev.c
 * @brief Linux evdev 触摸输入的用户态封装（本程序触摸数据的入口）。
 *        工作流程：evdev_init/evdev_set_file 打开 /dev/input/eventX 节点后，
 *        创建后台线程 evdev_thread 用 select 阻塞等待内核上报 input_event，
 *        把最新触点存进共享缓存；LVGL 线程周期调用 evdev_read()，
 *        经旋转、校准映射、裁剪、钳位后把坐标交给 LVGL。
 *        线程模型类比：evdev_thread ≈ 裸机里的“触摸采集任务”，
 *        select ≈ 等中断标志位，evdev_lock 互斥锁 ≈ 关中断保护的临界区。
 */

/*********************
 *      INCLUDES
 *********************/
#include "evdev.h"
/* 整个文件受 USE_EVDEV 开关控制：为 0 时本文件不参与编译 */
#if USE_EVDEV != 0 || USE_BSD_EVDEV

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
/* libevdev：对 /dev/input 的封装库，用来查询设备能力（如坐标范围），免得手写 ioctl */
#include <libevdev-1.0/libevdev/libevdev.h>
#include <dirent.h>

#define KALMAN_FILTER_EN    0    /* 卡尔曼滤波开关：1 时对坐标做平滑滤波（默认关闭） */
#if KALMAN_FILTER_EN
#include "kalman_filter.h"
#endif
/*********************
 *      DEFINES
 *********************/

/* 接近/光传感器的 ioctl 命令字（仅 USE_SENSOR=1 时使用，与内核驱动约定一致） */
#define PSENSOR_IOCTL_MAGIC             'p'
#define PSENSOR_IOCTL_GET_ENABLED       _IOR(PSENSOR_IOCTL_MAGIC, 1, int *)
#define PSENSOR_IOCTL_ENABLE            _IOW(PSENSOR_IOCTL_MAGIC, 2, int *)
#define PSENSOR_IOCTL_DISABLE           _IOW(PSENSOR_IOCTL_MAGIC, 3, int *)

#define LIGHTSENSOR_IOCTL_MAGIC         'l'
#define LIGHTSENSOR_IOCTL_GET_ENABLED   _IOR(LIGHTSENSOR_IOCTL_MAGIC, 1, int *)
#define LIGHTSENSOR_IOCTL_ENABLE        _IOW(LIGHTSENSOR_IOCTL_MAGIC, 2, int *)
#define LIGHTSENSOR_IOCTL_SET_RATE      _IOW(LIGHTSENSOR_IOCTL_MAGIC, 3, short)

/**********************
 *      TYPEDEFS
 **********************/

/* 采集线程与 evdev_read 之间传递的一帧触摸数据 */
typedef struct evdev_record
{
    int x;          /**< 触摸点原始 x 坐标（驱动上报值，未映射） */
    int y;          /**< 触摸点原始 y 坐标（驱动上报值，未映射） */
    int button;     /**< 按下状态：LV_INDEV_STATE_PR 按下 / LV_INDEV_STATE_REL 抬起 */
} evdev_record_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
int map(int x, int in_min, int in_max, int out_min, int out_max);

/**********************
 *  STATIC VARIABLES
 **********************/
static struct libevdev *evdev = NULL;             /**< libevdev 句柄，用于查询设备能力 */
static int evdev_fd = -1;                         /**< 触摸设备节点的文件描述符（≈ MCU 打开外设句柄） */
static int evdev_min_x = DEFAULT_EVDEV_HOR_MIN;   /**< 触摸原始 x 最小值（来自驱动上报） */
static int evdev_max_x = DEFAULT_EVDEV_HOR_MAX;   /**< 触摸原始 x 最大值 */
static int evdev_min_y = DEFAULT_EVDEV_VER_MIN;   /**< 触摸原始 y 最小值 */
static int evdev_max_y = DEFAULT_EVDEV_VER_MAX;   /**< 触摸原始 y 最大值 */
static int evdev_calibrate = 0;                   /**< 1：原始坐标范围≠屏幕分辨率，需要线性映射 */
static evdev_record_t evdev_val[2];               /**< 双缓冲：[1] 最新点，[0] 上一点（用于识别新按下） */
static int touch_up = 0;                          /**< 1：采集线程已收到抬起事件，等待 evdev_read 消费 */
static pthread_t evdev_tid;                       /**< 触摸采集线程句柄（pthread ≈ 裸机里的任务） */
static pthread_mutex_t evdev_lock;                /**< 保护 evdev_val 的互斥锁（≈ 临界区） */
static int touch_crop = 0;                        /**< 1：触摸范围大于屏幕，按居中窗口裁剪而非映射 */
static int crop_x1;                               /**< 裁剪窗口左上角 x */
static int crop_y1;                               /**< 裁剪窗口左上角 y */
static int crop_x2;                               /**< 裁剪窗口右下角 x */
static int crop_y2;                               /**< 裁剪窗口右下角 y */

static int evdev_key_val;                         /**< 键盘设备最近一次按键对应的 LVGL 键值 */
static int evdev_button;                          /**< 按键/触摸的按下状态（KEYPAD 类型设备用） */

static int evdev_rot;                             /**< 触摸坐标旋转角度（0/90/180/270），由 evdev_init 传入 */

#define FILE_DEBUG      0    /* 调试开关：1 时把原始/映射后坐标写到 /tmp 下的日志文件 */
#if FILE_DEBUG
static FILE *raw_point;
static FILE *lv_raw_point;
static FILE *lv_fix_point;
#endif

#if KALMAN_FILTER_EN
static KalmanFilter kfx;    /* x 轴卡尔曼滤波器状态 */
static KalmanFilter kfy;    /* y 轴卡尔曼滤波器状态 */
#endif

#if USE_SENSOR
static int psensor_event_id = -1;   /* 接近传感器对应的 input 设备编号，-1 表示尚未探测到 */
static int lsensor_event_id = -1;   /* 光传感器对应的 input 设备编号，-1 表示尚未探测到 */
static int psensor_fd = -1;         /* 接近传感器 event 节点 fd */
static int lsensor_fd = -1;         /* 光传感器 event 节点 fd */
#endif
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#define TP_NAME_LEN (32)    /* 设备节点路径/名称缓冲长度 */
/* 实际使用的触摸设备节点路径：初值为 EVDEV_NAME，会被环境变量或自动探测结果覆盖 */
static char tp_event[TP_NAME_LEN] = EVDEV_NAME;
static void *evdev_thread(void *arg);

/**
 * @brief 读取环境变量并解析为 uint32（支持 "0x" 前缀的十六进制写法）。
 * @param name          环境变量名
 * @param value         输出解析结果
 * @param default_value 变量不存在或解析失败时使用的默认值
 * @return 固定返回 0
 * @note 解析失败时会清掉 errno，避免把错误状态泄漏给调用者；本文件暂未调用（预留接口）。
 */
static int32_t env_get_u32(const char *name, uint32_t *value, uint32_t default_value)
{
    char *ptr = getenv(name);

    if (NULL == ptr)
    {
        *value = default_value;
    }
    else
    {
        char *endptr;
        int base = (ptr[0] == '0' && ptr[1] == 'x') ? (16) : (10);
        errno = 0;
        *value = strtoul(ptr, &endptr, base);
        if (errno || (ptr == endptr))
        {
            errno = 0;
            *value = default_value;
        }
    }

    return 0;
}

/**
 * Get touchscreen device event no
 * @brief 自动探测触摸屏的 event 节点：遍历 sysfs 的 /sys/class/input/inputX/name，
 *        找名字含 "ts"（touchscreen）或 "gsl"（GSL 系列触摸芯片）的设备，
 *        把匹配到的 "/dev/input/eventN" 写进全局 tp_event。
 *        /sys/class/input 是内核导出的设备信息目录，相当于只读的“设备登记表”。
 * @return 0 找到触摸设备；-1 未找到或遍历失败
 * @note 这里默认 inputN 与 eventN 编号一致（本板内核满足该约定）；
 *       USE_SENSOR=1 时顺带记录接近/光传感器的编号。
 */
int evdev_get_tp_event(void)
{
    int fd = 0, len = 0;
    int i = 0, input_dev_num = 0;
    DIR *pDir;
    struct dirent *ent = NULL;
    char file_name[TP_NAME_LEN];
    char tp_name[TP_NAME_LEN];
    char *path = "/sys/class/input";

    if ((pDir = opendir(path)) == NULL)
    {
        printf("%s: open %s filed\n", __func__, path);
        return -1;
    }

    while ((ent = readdir(pDir)) != NULL)
    {
        /* 先数一遍有多少个 inputX 目录，决定下面的扫描范围 */
        if (strstr(ent->d_name, "input"))
            input_dev_num++;
//            printf("%s: %s input deveices %d\n",
//                   __func__,
//                   ent->d_name,
//                   input_dev_num);
    }
    closedir(pDir);

    for (i = 0; i < input_dev_num; i++)
    {
        /* 逐个读 /sys/class/input/inputN/name，内容是驱动上报的设备名字符串 */
        sprintf(file_name, "/sys/class/input/input%d/name", i);
        fd = open(file_name, O_RDONLY);
        if (fd == -1)
        {
            printf("%s: open %s failed\n", __func__, file_name);
            continue;
        }

        len = read(fd, tp_name, TP_NAME_LEN);
        close(fd);
        if (len <= 0)
        {
            printf("%s: read %s failed\n", __func__, file_name);
            continue;
        }

        if (len >= TP_NAME_LEN)
            len = TP_NAME_LEN - 1;

        tp_name[len] = '\0';    /* sysfs 读出的名字自带换行，截断补 '\0' 后再做子串匹配 */

#if USE_SENSOR
        /* 先识别传感器并记录编号，避免被下面的 "ts" 规则误判成触摸屏 */
        if (strstr(tp_name, "lightsensor"))
        {
            lsensor_event_id = i;
            continue;
        }

        if (strstr(tp_name, "proximity"))
        {
            psensor_event_id = i;
            continue;
        }
#else
        /*
         * There is a 'ts' in the 'lightsensor', skip it to avoid being
         * treated as a touch device
         */
        if (strstr(tp_name, "lightsensor"))
            continue;
#endif

        /* "ts"≈touchscreen，"gsl"≈GSL 触摸芯片：命中即认为是触摸设备 */
        if (strstr(tp_name, "ts") || strstr(tp_name, "gsl"))
        {
            sprintf(tp_event, "/dev/input/event%d", i);
            printf("%s: %s = %s%s\n", __func__, file_name, tp_name, tp_event);
            return 0;
        }
    }

    return -1;
}

#if USE_SENSOR
/**
 * @brief 传感器输入设备的 read_cb：非阻塞轮询接近/光传感器事件。
 * @param drv  LVGL 输入设备驱动指针，user_data 里存的是传感器 fd 的地址
 * @param data 输出：state（接近=按下/远离=抬起）、key（光强值）
 * @note 用超时为 0 的 select 判断有无数据，没有就直接返回，不阻塞 LVGL 线程；
 *       continue_reading=1 表示可能还有数据，提示 LVGL 继续调用。
 */
void evdev_sensor_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    struct input_event in;
    struct timeval tv;
    int drv_fd = *(int *)drv->user_data;
    fd_set rdfs;
    int ret;

    FD_ZERO(&rdfs);
    FD_SET(drv_fd, &rdfs);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    data->continue_reading = 1;
    if (select(drv_fd + 1, &rdfs, NULL, NULL, &tv) <= 0)
    {
        return;
    }

    if ((ret = read(drv_fd, &in, sizeof(in))) > 0)
    {
        if (in.type == EV_ABS)
        {
            if (in.code == ABS_DISTANCE)
            {
                if (in.value == 0)
                    data->state = LV_INDEV_STATE_RELEASED;
                else
                    data->state = LV_INDEV_STATE_PRESSED;
                data->continue_reading = 0;
            }
            else if (in.code == ABS_MISC)
            {
                if (in.value == 0)
                    data->state = LV_INDEV_STATE_RELEASED;
                else
                    data->state = LV_INDEV_STATE_PRESSED;
                data->key = in.value;
                data->continue_reading = 0;
            }
        }
    }
}

/**
 * @brief 返回接近传感器 fd 变量的地址，供 drv->user_data 使用。
 */
void *evdev_get_psensor(void)
{
    return &psensor_fd;
}

/**
 * @brief 初始化接近传感器：先通过 ioctl 使能 /dev/psensor，再打开对应的 event 节点。
 * @return ioctl 的返回值（>=0 成功）；失败返回 -1
 * @note 使能用的是厂商私有字符设备 /dev/psensor，数据却走标准 event 节点上报，
 *       两者要分别打开。
 */
int evdev_init_psensor(void)
{
    char event_name[64];
    int fd, ret, enable = 1;

    if (psensor_event_id == -1)
    {
        if (evdev_get_tp_event() < 0)
        {
            printf("%s get event failed\n", __func__);
            return -1;
        }
        if (psensor_event_id == -1)
        {
            printf("%s get psensor event failed\n", __func__);
            return -1;
        }
    }

    fd = open("/dev/psensor", O_RDWR);
    if (fd < 0)
    {
        printf("can't open /dev/psensor!\n");
        return -1;
    }

    ret = ioctl(fd, PSENSOR_IOCTL_ENABLE, &enable);
    if (ret < 0)
    {
        printf("eanble /dev/psensor failed %d!\n", ret);
    }
    else
    {
        printf("enable /dev/psensor successfully!\n");
    }
    close(fd);

    snprintf(event_name, sizeof(event_name),
             "/dev/input/event%d", psensor_event_id);
    fd = open(event_name, O_RDONLY);
    if (fd < 0)
    {
        printf("can't open %s\n", event_name);
        return -1;
    }
    psensor_fd = fd;

    return ret;
}

/**
 * @brief 返回光传感器 fd 变量的地址，供 drv->user_data 使用。
 */
void *evdev_get_lsensor(void)
{
    return &lsensor_fd;
}

/**
 * @brief 初始化光传感器：先通过 ioctl 使能 /dev/lightsensor，再打开对应的 event 节点。
 * @return ioctl 的返回值（>=0 成功）；失败返回 -1
 * @note 结构同 evdev_init_psensor()：私有节点做控制，event 节点收数据。
 */
int evdev_init_lsensor(void)
{
    char event_name[64];
    int fd, ret, enable = 1;

    if (lsensor_event_id == -1)
    {
        if (evdev_get_tp_event() < 0)
        {
            printf("%s get event failed\n", __func__);
            return -1;
        }
        if (lsensor_event_id == -1)
        {
            printf("%s get lsensor event failed\n", __func__);
            return -1;
        }
    }

    fd = open("/dev/lightsensor", O_RDWR);
    if (fd < 0)
    {
        printf("can't open /dev/lightsensor!\n");
        return -1;
    }

    ret = ioctl(fd, LIGHTSENSOR_IOCTL_ENABLE, &enable);
    if (ret < 0)
    {
        printf("eanble /dev/lightsensor failed %d!\n", ret);
    }
    else
    {
        printf("enable /dev/lightsensor successfully!\n");
    }
    close(fd);

    snprintf(event_name, sizeof(event_name),
             "/dev/input/event%d", lsensor_event_id);
    fd = open(event_name, O_RDONLY);
    if (fd < 0)
    {
        printf("can't open %s\n", event_name);
        return -1;
    }
    lsensor_fd = fd;

    return ret;
}
#endif

/**
 * Initialize the evdev interface
 * @brief 初始化触摸输入入口：确定设备节点（环境变量优先，否则自动探测），
 *        再交给 evdev_set_file() 完成打开与线程创建。
 * @param drv  LVGL 显示驱动指针（取屏幕分辨率用）
 * @param rot  触摸坐标旋转角度（0/90/180/270），存到全局 evdev_rot
 * @return 0 成功；-1 未找到触摸设备或打开失败
 */
int evdev_init(lv_disp_drv_t *drv, int rot)
{
    const char *event_name;
    int rc = 1;
    evdev_rot = rot;

#if FILE_DEBUG
    raw_point = fopen("/tmp/raw_point.txt", "wb+");
    lv_raw_point = fopen("/tmp/lv_raw_point.txt", "wb+");
    lv_fix_point = fopen("/tmp/lv_fix_point.txt", "wb+");
#endif

    /* 调试/适配时可用环境变量 LV_EVENT_NAME 直接指定节点，跳过自动探测 */
    event_name = getenv("LV_EVENT_NAME");
    if (event_name)
    {
        strncpy(tp_event, event_name, TP_NAME_LEN);
    }
    else if (evdev_get_tp_event() < 0)
    {
        printf("%s get tp event failed\n", __func__);
        return -1;
    }
    return evdev_set_file(drv, tp_event);
}
/**
 * reconfigure the device file for evdev
 * @brief 打开（或换绑）evdev 设备节点，并按需创建采集线程。
 *        流程：关闭旧句柄 → 打开新节点 → 用 libevdev 读取触摸坐标范围 →
 *        根据“原始范围 vs 屏幕分辨率”决定做线性映射（calibrate）还是居中裁剪（crop）→
 *        初始化互斥锁并启动 evdev_thread。
 * @param drv      LVGL 显示驱动指针（取 hor_res/ver_res）
 * @param dev_name set the evdev device filename（如 "/dev/input/event2"）
 * @return true: the device file set complete
 *         false: the device file doesn't exist current system
 * @note 实际返回 0 成功 / -1 打开失败（旧注释的 true/false 不准确）；
 *       互斥锁/线程在重复调用时会重复 init/create，正常流程只调用一次。
 */
int evdev_set_file(lv_disp_drv_t *drv, char *dev_name)
{
    int disp_hor;
    int disp_ver;
    int rc = 1;

    /* 支持重复调用换绑设备：先释放旧资源再打开新节点 */
    if (evdev_fd != -1)
        close(evdev_fd);
    evdev_fd = -1;
    if (evdev)
        libevdev_free(evdev);
    evdev = NULL;

#if USE_BSD_EVDEV
    evdev_fd = open(dev_name, O_RDWR | O_NOCTTY);
#else
    /* O_NDELAY：非阻塞打开，配合后面的 O_NONBLOCK，read 没数据时立即返回 */
    evdev_fd = open(dev_name, O_RDWR | O_NOCTTY | O_NDELAY);
#endif

    if (evdev_fd == -1)
    {
        perror("unable open evdev interface:");
        return -1;
    }

    rc = libevdev_new_from_fd(evdev_fd, &evdev);
    if (rc < 0)
    {
        printf("Failed to init libevdev (%s)\n", strerror(-rc));
    }
    else
    {
        /* 设备支持绝对坐标（EV_ABS）才有范围可查；鼠标类相对坐标设备走默认范围 */
        if (libevdev_has_event_type(evdev, EV_ABS))
        {
            const struct input_absinfo *abs;
            /* 读取驱动上报的多点触摸 X/Y 坐标范围，存到 evdev_min/max_x/y。
             * 注意旋转 90/270 时屏幕的 x 对应触摸的 y，所以范围也要交叉赋值 */
            if (libevdev_has_event_code(evdev, EV_ABS, ABS_MT_POSITION_X))
            {
                abs = libevdev_get_abs_info(evdev, ABS_MT_POSITION_X);
                printf("EV_ABS ABS_MT_POSITION_X\n");
                printf("\tMin\t%6d\n", abs->minimum);
                printf("\tMax\t%6d\n", abs->maximum);
                if ((evdev_rot == 0) || (evdev_rot == 180))
                {
                    evdev_min_x = abs->minimum;
                    evdev_max_x = abs->maximum;
                }
                else
                {
                    evdev_min_y = abs->minimum;
                    evdev_max_y = abs->maximum;
                }
            }
            if (libevdev_has_event_code(evdev, EV_ABS, ABS_MT_POSITION_Y))
            {
                abs = libevdev_get_abs_info(evdev, ABS_MT_POSITION_Y);
                printf("EV_ABS ABS_MT_POSITION_Y\n");
                printf("\tMin\t%6d\n", abs->minimum);
                printf("\tMax\t%6d\n", abs->maximum);
                if ((evdev_rot == 0) || (evdev_rot == 180))
                {
                    evdev_min_y = abs->minimum;
                    evdev_max_y = abs->maximum;
                }
                else
                {
                    evdev_min_x = abs->minimum;
                    evdev_max_x = abs->maximum;
                }
            }
        }
    }

    /* 采集线程里用 select 等数据、read 一次性掏空，fd 必须保持非阻塞 */
    fcntl(evdev_fd, F_SETFL, O_ASYNC | O_NONBLOCK);

    /* 清空共享缓存，初始状态置为“抬起”，避免上电就误报一次按下 */
    memset(evdev_val, 0, sizeof(evdev_val));
    evdev_key_val = 0;
    evdev_button = LV_INDEV_STATE_REL;

    disp_hor = drv->hor_res;
    disp_ver = drv->ver_res;
    /* 触摸原始范围与屏幕分辨率不一致时，两种处理策略二选一：
     * 1) 默认做线性映射（calibrate），把全触摸范围拉到全屏；
     * 2) 若环境变量 lv_disp_crop 非 0，则按屏幕大小居中裁剪（crop），
     *    适用于触摸面板物理上比显示区大一圈的场景 */
    if ((evdev_min_x != 0) ||
            (evdev_max_x != disp_hor) ||
            (evdev_min_y != 0) ||
            (evdev_max_y != disp_ver))
    {
        const char *buf;
        buf = getenv("lv_disp_crop");
        if (buf)
            touch_crop = buf[0] - '0';
        if (!touch_crop)
        {
            evdev_calibrate = 1;
            printf("calibrate [%d,%d]x[%d,%d] to %dx%d\n",
                   evdev_min_x, evdev_max_x,
                   evdev_min_y, evdev_max_y,
                   disp_hor, disp_ver);
        }
        else
        {
            /* 居中裁剪：取触摸范围正中间一块与屏幕等大的区域 */
            crop_x1 = (evdev_max_x - disp_hor) / 2;
            crop_y1 = (evdev_max_y - disp_ver) / 2;
            crop_x2 = crop_x1 + disp_hor;
            crop_y2 = crop_y1 + disp_ver;
            printf("crop [%d,%d]x[%d,%d] to [%d,%d]x[%d,%d]\n",
                   evdev_min_x, evdev_max_x,
                   evdev_min_y, evdev_max_y,
                   crop_x1, crop_y1, crop_x2, crop_y2);
        }
    }
    printf("evdev_calibrate = %d\n", evdev_calibrate);

    /* 启动后台采集线程：它独占 read 设备节点，结果经互斥锁保护的 evdev_val 交给 LVGL */
    pthread_mutex_init(&evdev_lock, NULL);
    pthread_create(&evdev_tid, NULL, evdev_thread, NULL);

    return 0;
}
/**
 * Get the current position and state of the evdev
 * @param data store the evdev data here
 * @return false: because the points are not buffered, so no more data to be read
 * @note 上面这段是 evdev_read() 的注释（位置放这里了，描述的是读取语义），保留原样。
 */

/**
 * @brief 触摸采集线程主函数：独占读取设备节点，维护共享缓存 evdev_val。
 *        线程模型 ≈ 裸机里专门等触摸中断的任务；select 阻塞等待 ≈ 等中断标志，
 *        有数据后内层 while 把内核缓冲区里的事件一次性读空。
 * @param arg 未使用（pthread 入口签名要求）
 * @return 实际不会返回（while(1) 循环，线程随进程退出而结束）
 * @note 本线程不调用任何 LVGL API（LVGL 非线程安全），只写共享缓存；
 *       与 evdev_read 之间的数据交换用 evdev_lock 保护。
 */
static void *evdev_thread(void *arg)
{
    int type = LV_INDEV_TYPE_POINTER;
    struct input_event in;
    int x = 0;
    int y = 0;
    int button = 0;
    fd_set rdfs;

    FD_ZERO(&rdfs);
    FD_SET(evdev_fd, &rdfs);

    while (1)
    {
        /* 阻塞等待 fd 可读（没有超时参数：睡到内核报事件为止，不耗 CPU） */
        select(evdev_fd + 1, &rdfs, NULL, NULL, NULL);

        /* fd 是非阻塞的：循环 read 直到返回 <=0，把这一批事件全部处理完 */
        while (read(evdev_fd, &in, sizeof(struct input_event)) > 0)
        {
            if (in.type == EV_REL)      /* 相对坐标事件（鼠标类设备） */
            {
                if (in.code == REL_X)
#if EVDEV_SWAP_AXES
                    y += in.value;
#else
                    x += in.value;
#endif
                else if (in.code == REL_Y)
#if EVDEV_SWAP_AXES
                    x += in.value;
#else
                    y += in.value;
#endif
            }
            else if (in.type == EV_ABS) /* 绝对坐标事件（触摸屏） */
            {
                if (in.code == ABS_X)
#if EVDEV_SWAP_AXES
                    y = in.value;
#else
                    x = in.value;
#endif
                else if (in.code == ABS_Y)
#if EVDEV_SWAP_AXES
                    x = in.value;
#else
                    y = in.value;
#endif
                else if (in.code == ABS_MT_POSITION_X)
#if EVDEV_SWAP_AXES
                    y = in.value;
#else
                    x = in.value;
#endif
                else if (in.code == ABS_MT_POSITION_Y)
#if EVDEV_SWAP_AXES
                    x = in.value;
#else
                    y = in.value;
#endif
                else if (in.code == ABS_MT_TRACKING_ID)
                {
                    /* 多点触摸协议（type B）：TRACKING_ID = -1 表示该触点抬起，
                     * 否则表示按下/更新；用它来生成按下/抬起状态 */
                    if (in.value == -1)
                    {
                        button = LV_INDEV_STATE_REL;
                        touch_up = 1;   /* 抬起事件先记账，等 evdev_read 消费（防漏检，见 evdev_read） */
                    }
                    else/* if ((in.value == 0) || (in.value == 1)) */
                    {
                        button = LV_INDEV_STATE_PR;
                    }
                }
            }
            else if (in.type == EV_KEY)
            {
                /* 触摸/鼠标按键：value=1 按下、0 抬起（单点触摸靠它报按下状态） */
                if (in.code == BTN_MOUSE || in.code == BTN_TOUCH)
                {
                    if (in.value == 0)
                    {
                        evdev_button = LV_INDEV_STATE_REL;
                        button = LV_INDEV_STATE_REL;
                    }
                    else if (in.value == 1)
                    {
                        evdev_button = LV_INDEV_STATE_PR;
                        button = LV_INDEV_STATE_PR;
                    }
                }
                else if (type == LV_INDEV_TYPE_KEYPAD)
                {
                    /* 把 Linux 键值翻译成 LVGL 键值（本工程用 POINTER 类型，默认不走这里） */
                    switch (in.code)
                    {
                    case KEY_BACKSPACE:
                        evdev_key_val = LV_KEY_BACKSPACE;
                        break;
                    case KEY_ENTER:
                        evdev_key_val = LV_KEY_ENTER;
                        break;
                    case KEY_UP:
                        evdev_key_val = LV_KEY_UP;
                        break;
                    case KEY_LEFT:
                        evdev_key_val = LV_KEY_PREV;
                        break;
                    case KEY_RIGHT:
                        evdev_key_val = LV_KEY_NEXT;
                        break;
                    case KEY_DOWN:
                        evdev_key_val = LV_KEY_DOWN;
                        break;
                    default:
                        evdev_key_val = 0;
                        break;
                    }
                }
            }
        }

        /* 一批事件处理完，加锁把最新点推进双缓冲：
         * [1] 始终是最新点，[0] 是它的前一个状态，
         * evdev_read 靠比较 [0]/[1] 的 button 识别“抬起后第一次按下” */
        pthread_mutex_lock(&evdev_lock);
        evdev_val[0] = evdev_val[1];
        evdev_val[1].x = x;
        evdev_val[1].y = y;
        evdev_val[1].button = button;
        pthread_mutex_unlock(&evdev_lock);

#if FILE_DEBUG
        fprintf(raw_point, "%d\t%d\t%d\n", x, y, button);
        fflush(raw_point);
#endif
    }

    return NULL;
}

/**
 * @brief LVGL 输入设备的 read_cb：由 LVGL 线程（lv_timer_handler）周期调用，
 *        从共享缓存取出最新触点，做旋转、校准映射、裁剪、边界钳位后填给 data。
 * @param drv  LVGL 输入设备驱动指针（取设备类型和屏幕分辨率）
 * @param data 输出：point.x/point.y 屏幕坐标，state 按下/抬起状态
 * @note LVGL 本身非线程安全：本函数只在 LVGL 线程里跑，持锁仅用于
 *       拷贝 evdev_val，不做任何耗时操作，避免拖慢采集线程。
 */
void evdev_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    int type = drv->type;
    lv_disp_t *disp = drv->disp;
    int hor_res = disp->driver->hor_res;
    int ver_res = disp->driver->ver_res;
    int raw_x, raw_y, button_state;
    int x, y;
    int tmp;
    int first_point = 0;    /* 1：本次是“抬起后第一次按下”的首点（卡尔曼滤波需重置） */

    if (type == LV_INDEV_TYPE_KEYPAD)
    {
        /* No data retrieved */
        /* 键盘设备：直接把采集线程翻译好的键值/状态交给 LVGL */
        data->key = evdev_key_val;
        data->state = evdev_button;
        return;
    }

    if (type != LV_INDEV_TYPE_POINTER)
        return;
    /*Store the collected data*/
    /* 加锁拷贝共享缓存：与采集线程的唯一交互点，快进快出 */
    pthread_mutex_lock(&evdev_lock);

    raw_x = evdev_val[1].x;
    raw_y = evdev_val[1].y;
    button_state = evdev_val[1].button;
    if (evdev_val[0].button == LV_INDEV_STATE_REL)
    {
        /* 上一帧是抬起：当前帧是新一轮按下的第一个点 */
        first_point = 1;
        touch_up = 0;
    }
    else if (button_state && touch_up)
    {
        /* Some times evdev_read may lost the touch up events due to
         * the renderer has different period with touch screen report events,
         * so add a global variable touch_up to make sure the evdev_read will
         * not lost any touch up events.
         */
        /* 中文说明：LVGL 刷新周期与触摸上报周期不同步时，快速的“点一下”可能在
         * 两次 evdev_read 之间就完成了按下+抬起，双缓冲里只看到按下，抬起被吞掉，
         * 会导致 LVGL 以为一直按着。采集线程用 touch_up 记账，这里发现
         * “记账了但状态又是按下”，就补判一次首点，保证抬起/按下不丢失 */
        first_point = 1;
        touch_up = 0;
    }

    pthread_mutex_unlock(&evdev_lock);

    /* 按初始化时指定的角度旋转原始坐标，使触摸方向与显示方向一致 */
    switch (evdev_rot)
    {
    case 0:
    default:
        break;
    case 90:
        tmp = raw_x;
        raw_x = raw_y;
        raw_y = evdev_max_y - tmp;
        break;
    case 180:
        tmp = raw_x;
        raw_x = evdev_max_x - raw_y;
        raw_y = evdev_max_y - tmp;
        break;
    case 270:
        tmp = raw_x;
        raw_x = evdev_max_x - raw_y;
        raw_y = tmp;
        break;
    }

    /* 触摸原始范围 ≠ 屏幕分辨率时，线性映射到屏幕坐标 */
    if (evdev_calibrate)
    {
        raw_x = map(raw_x, evdev_min_x, evdev_max_x,
                    0, hor_res);
        raw_y = map(raw_y, evdev_min_y, evdev_max_y,
                    0, ver_res);
    }

#if KALMAN_FILTER_EN
    KalmanUpdate(&kfx, (float)raw_x / evdev_max_x, first_point);
    KalmanUpdate(&kfy, (float)raw_y / evdev_max_y, first_point);

    x = (int)(kfx.v * evdev_max_x);
    y = (int)(kfy.v * evdev_max_y);
#else
    x = raw_x;
    y = raw_y;
#endif

    /* 居中裁剪模式：把坐标钳到裁剪窗口内再平移到屏幕原点 */
    if (touch_crop)
    {
        if (x < crop_x1)
            x = crop_x1;
        else if (x > crop_x2)
            x = crop_x2;
        x -= crop_x1;

        if (y < crop_y1)
            y = crop_y1;
        else if (y > crop_y2)
            y = crop_y2;
        y -= crop_y1;
    }

    data->point.x = x;
    data->point.y = y;
    data->state = button_state;

    /* 边界钳位：保证交给 LVGL 的坐标一定落在 [0, 分辨率-1] 内 */
    if (data->point.x < 0)
        data->point.x = 0;
    if (data->point.y < 0)
        data->point.y = 0;
    if (data->point.x >= hor_res)
        data->point.x = hor_res - 1;
    if (data->point.y >= ver_res)
        data->point.y = ver_res - 1;

#if FILE_DEBUG
    if (data->state)
    {
        fprintf(lv_raw_point, "%d\t%d\t%d\n", raw_x, raw_y, button_state);
        fflush(lv_raw_point);
        fprintf(lv_fix_point, "%d\t%d\t%d\n", data->point.x, data->point.y,
                data->state);
        fflush(lv_fix_point);
    }
#endif

    return ;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
/**
 * @brief 线性映射：把 x 从 [in_min, in_max] 区间等比换算到 [out_min, out_max] 区间。
 *        与 Arduino 的 map() 相同，用于触摸原始坐标 → 屏幕坐标。
 * @note 纯整数运算（先乘后除减小精度损失）；in_max == in_min 时会除零，
 *       调用前已保证范围有效。
 */
int map(int x, int in_min, int in_max, int out_min, int out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif
