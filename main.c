/*
 * Copyright (c) 2021 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*
 * Copyright (c) 2025 广州市星翼电子科技有限公司（正点原子）All Rights Reserved.
 *
 * @author: Deng Zhimao
 * @email: dengzhimao@alientek.com
 * B站作品: https://space.bilibili.com/474103963?spm_id_from=333.788.0.0

 * 开源电子网: http://www.openedv.com/forum.php
 * 正点原子官网: https://www.alientek.com
 * 店铺: https://zhengdianyuanzi.tmall.com
 *
 * LICENCE GPLV3
 */

/**
 * @file main.c
 * @brief 程序入口：完成 LVGL 移植层初始化（DRM 显示 + evdev 触摸）、
 *        FreeType 中文字体加载、LED 心跳灯控制，最后进入主循环驱动 GUI。
 * @note  界面代码由 NXP GUI Guider 生成（见 user/ 目录），本文件只负责
 *        “初始化 + 主循环”。LVGL 本身非线程安全，本程序所有 LVGL API
 *        都只在主线程里调用；其他线程（如蓝牙扫描线程）不直接操作 GUI，
 *        而是借助 lv_timer 在主线程内周期刷新界面。
 */

#include "main.h"
#include <lvgl/lv_conf.h>
#include <lvgl/lvgl.h>
#include "gui_guider.h"
#include <unistd.h>
static int quit = 0;    // 退出标志：信号处理函数里置 1，主循环轮询到后退出

// 字库结构体实例
lv_ft_info_t ft_info;
// 按钮
static lv_obj_t *btn;
// 标签
static lv_obj_t *label;

// 开发板中字体库路径
#define FREETYPE_FONT_FILE ("/usr/share/fonts/source-han-sans-cn/SourceHanSansCN-Normal.otf")    /**< 思源黑体，由 FreeType 在运行时从文件加载，需事先部署到根文件系统 */

#define LED_BRIGHTNESS_PATH "/sys/class/leds/work/brightness"   /**< 板上 work 灯的亮度节点，写 0 灭、写 1 亮（/sys 节点 ≈ 外设寄存器的文件化接口） */

/**
 * @brief SIGINT（Ctrl+C）信号处理函数
 * @param sig 触发本处理函数的信号编号
 * @note  信号处理函数 ≈ 裸机里的中断服务程序，应尽可能短：这里只置 quit
 *        标志，真正的退出动作交给主循环轮询完成。
 *        （fprintf 严格来说不是异步信号安全函数，此处为调试打印保留。）
 */
static void sigterm_handler(int sig) {
    fprintf(stderr, "signal %d\n", sig);
    quit = 1;
}

/**
 * @brief 读取 work 灯当前亮度值
 * @return 成功返回亮度值（0 = 灭，非 0 = 亮）；失败返回 -1
 * @note  走内核 LED 子系统的 sysfs 接口，fopen/fscanf 读写节点
 *        ≈ 裸机读写寄存器，只是换成了文件 IO 的形式。
 */
// 读取led状态
int read_brightness() {
    FILE *file = fopen(LED_BRIGHTNESS_PATH, "r");
    if (!file) {
        LV_LOG_USER("Failed to open brightness file for reading");
        return -1;
    }
    int brightness;
    if (fscanf(file, "%d", &brightness) != 1) {
        LV_LOG_USER("Failed to read brightness value");
        fclose(file);
        return -1;
    }
    fclose(file);
    return brightness;
}

/**
 * @brief 写 work 灯亮度值，控制 LED 亮灭
 * @param brightness 亮度值，0 = 灭，非 0 = 亮
 * @return 成功返回 0；失败返回 -1
 * @note  写之前必须先把该灯的 trigger 设为 none（见 main()），
 *        否则内核的 heartbeat 触发器会自动翻转亮度，覆盖这里写入的值。
 */
// 控制led
int write_brightness(int brightness) {
    FILE *file = fopen(LED_BRIGHTNESS_PATH, "w");
    if (!file) {
        LV_LOG_USER("Failed to open brightness file for writing");
        return -1;
    }

    if (fprintf(file, "%d\n", brightness) < 0) {
        LV_LOG_USER("Failed to write brightness value");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

// 下面整段是保留下来的“按钮点 LED”示例（已被注释，不参与编译）：
// 演示 LVGL 事件回调（LV_EVENT_CLICKED / LV_EVENT_VALUE_CHANGED）
// 与 write_brightness() 联动的写法，需要时可恢复参考。
// static void event_handler(lv_event_t *e) {
//     lv_event_code_t code = lv_event_get_code(e);
//     // 获取触发事件的对象
//     lv_obj_t *target = lv_event_get_target(e);

//     if (code == LV_EVENT_CLICKED) {
//         LV_LOG_USER("Clicked");
//         if (target == btn) {
//             LV_LOG_USER("btn Clicked");
//         }
//     }

//     if (code == LV_EVENT_VALUE_CHANGED) {
//         // 根据按钮状态更新标签文本
//         if (lv_obj_has_state(target, LV_STATE_CHECKED)) {
//             lv_label_set_text(label, "关灯");
//             write_brightness(1);
//         } else {
//             lv_label_set_text(label, "开灯");
//             write_brightness(0);
//         }
//     }
// }


/**
 * @brief 开机自检 SD 卡上的图片资源是否齐全
 * @note  界面用到的图片放在 /mnt/sdcard/icons/，缺失时 LVGL 只会显示空白，
 *        现象和程序 bug 很像。这里提前用 access(F_OK) 逐个检查并打印日志，
 *        便于现场区分“资源没拷进 SD 卡”还是程序问题；只告警不退出，
 *        缺图时程序仍可继续运行。
 */
static void check_image_files(void)
{
    const char *files[] = {
        "/mnt/sdcard/icons/ocean_720x1280.png",      // 主背景
        "/mnt/sdcard/icons/beach_720x730.png",       // 弹窗背景
        "/mnt/sdcard/icons/astronaut_720x1280.png",  // 主背景（另一张）
        "/mnt/sdcard/icons/cancer_100x100.png",      // 关闭按钮
        "/mnt/sdcard/icons/noconnect_100x100.png",   // 未连接图标
        "/mnt/sdcard/icons/connect_100x100.png",     // 已连接图标
        "/mnt/sdcard/icons/time_50x50.png",          // 时间图标
        "/mnt/sdcard/icons/scan_200x200.png",        // 扫描图标
        "/mnt/sdcard/icons/bird_144x159.png",        // 鸟图标 
    };
    int missing = 0;
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        if (access(files[i], F_OK) != 0) {
            LV_LOG_USER("Missing image file: %s", files[i]);
            missing++;
        }
    }
    if (missing == 0) {
        LV_LOG_USER("All image files found in /mnt/sdcard/icons/");
    } else {
        LV_LOG_USER("Warning: %d image file(s) missing", missing);
    }
}
/**
 * @brief 程序入口：初始化 LVGL、加载中文字体、构建并显示 GUI，进入主循环
 * @param argc 命令行参数个数（本程序未使用）
 * @param argv 命令行参数列表（本程序未使用）
 * @return 恒返回 0
 * @note  主循环 ≈ 裸机的 while(1)：反复调用 lv_task_handler()，由它统一
 *        处理 LVGL 的定时器、动画、输入事件和重绘送显。
 */
int main(int argc, char **argv) {
    // 捕获 Ctrl+C（SIGINT）：只置 quit 标志，让主循环自己走完当前帧再退出
    signal(SIGINT, sigterm_handler);

    // 一切LVGL应用的开始，必须加上这个初始化
    // 参数依次为宽、高、旋转角；全传 0 表示不指定宽高（由 DRM 驱动按屏幕
    // 实际分辨率 720x1280 处理）、不旋转
    lv_port_init(0, 0, 0);

    /*****************************用户程序开始*************************************/
    ft_info.name = FREETYPE_FONT_FILE; // 使用绝对路径指定字体文件
    ft_info.weight = 50;               // 字体大小
    ft_info.style = FT_FONT_STYLE_NORMAL;
    ft_info.mem = NULL;                // NULL 表示从字体文件加载，而非内存中的字库数据

    // 初始化字体
    // 注意：失败只打印提示不退出，后续用到该字体的控件会显示异常，调试时留意此打印
    if (!lv_ft_font_init(&ft_info)) {
        printf("create failed.");
    }

    // 将心跳改为手动触发
    // 内核默认给 work 灯配置了 heartbeat 触发器（自动闪烁），先设为 none 把
    // 控制权拿回来，否则下面手动写入的亮度会被内核覆盖；
    // system() 会 fork 一个 shell 执行命令，开销较大，仅在初始化时调用一次
    system("echo none > /sys/class/leds/work/trigger");

    // 上电先读一次当前亮度并打印，顺带验证 sysfs 通路是否正常
    int brightness = read_brightness();
    if (brightness != -1) {
        LV_LOG_USER("Current brightness: %d\n", brightness);
    }

    /*****************************NXP UI 初始化**********************************/
    // GUI Guider 生成的代码假定结构体已清零（靠指针是否为 NULL 判断控件是否
    // 已创建），必须先 memset，否则随机初值会导致野指针访问
    memset(&guider_ui, 0, sizeof(guider_ui));
    check_image_files();
    // 构建首页 screen 的控件对象树（此时尚未上屏）
    setup_scr_screen(&guider_ui);
    // 把 screen 切为当前活动屏幕，LVGL 从下一帧开始绘制它
    lv_scr_load(guider_ui.screen);
    /******************************结束******************************************/
    while (!quit) {
        /* 调用LVGL任务处理函数，LVGL所有的事件、绘制、送显等都在该接口内完成 */
        lv_task_handler();
        // 短暂休眠让出 CPU，避免死循环占满一个核；
        // LVGL 内部按 lv_timer 周期调度，这里的休眠时长不影响界面逻辑
        usleep(100);
    }

    return 0;
}
