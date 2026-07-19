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

#include "main.h"
#include <lvgl/lv_conf.h>
#include <lvgl/lvgl.h>
#include "gui_guider.h"
#include <unistd.h>
static int quit = 0;

// 字库结构体实例
lv_ft_info_t ft_info;
// 按钮
static lv_obj_t *btn;
// 标签
static lv_obj_t *label;

// 开发板中字体库路径
#define FREETYPE_FONT_FILE ("/usr/share/fonts/source-han-sans-cn/SourceHanSansCN-Normal.otf")

#define LED_BRIGHTNESS_PATH "/sys/class/leds/work/brightness"

static void sigterm_handler(int sig) {
    fprintf(stderr, "signal %d\n", sig);
    quit = 1;
}

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
int main(int argc, char **argv) {
    signal(SIGINT, sigterm_handler);

    // 一切LVGL应用的开始，必须加上这个初始化
    lv_port_init(0, 0, 0);

    /*****************************用户程序开始*************************************/
    ft_info.name = FREETYPE_FONT_FILE; // 使用绝对路径指定字体文件
    ft_info.weight = 50;               // 字体大小
    ft_info.style = FT_FONT_STYLE_NORMAL;
    ft_info.mem = NULL;

    // 初始化字体
    if (!lv_ft_font_init(&ft_info)) {
        printf("create failed.");
    }

    // 将心跳改为手动触发
    system("echo none > /sys/class/leds/work/trigger");

    int brightness = read_brightness();
    if (brightness != -1) {
        LV_LOG_USER("Current brightness: %d\n", brightness);
    }

    /*****************************NXP UI 初始化**********************************/
    memset(&guider_ui, 0, sizeof(guider_ui));
    check_image_files();
    setup_scr_screen(&guider_ui);
    lv_scr_load(guider_ui.screen);
    /******************************结束******************************************/
    while (!quit) {
        /* 调用LVGL任务处理函数，LVGL所有的事件、绘制、送显等都在该接口内完成 */
        lv_task_handler();
        usleep(100);
    }

    return 0;
}
