/**
 * @file main.h
 * @brief main.c 的配套头文件：集中包含程序所需的标准 C / POSIX 系统头文件
 *        （文件 IO、poll、pthread、mmap、ioctl 等 Linux 系统调用相关），
 *        并提供通用的地址对齐宏 ALIGN。
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <lvgl/lvgl.h>

#include "lv_port_init.h"   // LVGL 显示（DRM/KMS）+ 触摸（evdev）移植层初始化接口
#include "timestamp.h"      // 时间戳工具函数

// 把 x 向上对齐到 a 的整数倍（a 必须是 2 的幂），帧缓冲/内存地址对齐常用
#define ALIGN(x, a)     (((x) + (a - 1)) & ~(a - 1))

#endif

