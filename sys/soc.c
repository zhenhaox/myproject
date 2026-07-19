/**
 * @file soc.c
 * @brief SoC / 系统信息查询实现
 * @note  设备树节点 /proc/device-tree/compatible 由内核 dts 导出，
 *        内容是多个 '\0' 分隔的字符串（如 "rockchip,rv1126b\0rockchip,rv1126\0"），
 *        读出来需把 '\0' 换成空格才能作为整体打印；
 *        /proc/version 则是编译内核时生成的一整行版本描述
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "soc.h"

char *get_soc_name(char *buf)
{
    char *start;
    char *ret;
    int size;

    /* 定位型号起点（"rockchip,rv1126b" 中第一个 "rk" 恰好在型号前缀上，
     * 因为 "rockchip" 本身不含 "rk" 子串） */
    start = strstr(buf, "rk");
    ret = strdup(start);
    size = strlen(ret);

    /* 型号统一转大写输出，如 rv1126 -> RK1126；
     * 遇到非数字字符（含后缀字母）即截断，故 RV1126B 会显示为 "RK1126" */
    ret[0] = 'R';
    ret[1] = 'K';
    for (int i = 2; i < size; i++)
    {
        char c = *(ret + i);
        if (((c > '9') || (c < '0')) &&
                ((c != 'x') || (c != 'X')))
        {
            *(ret + i) = '\0';
            break;
        }
        *(ret + i) = toupper(c);
    }

    return ret;
}

char *get_compatible_name(void)
{
    const char *path = "/proc/device-tree/compatible";
    int fd = open(path, O_RDONLY);
    char name[128];
    int size = sizeof(name);
    ssize_t soc_name_len = 0;

    if (fd < 0)
    {
        printf("open %s error\n", path);
        return NULL;
    }

    /* 先填入 "unknown" 兜底：read 失败时仍有可返回的内容 */
    snprintf(name, size - 1, "unknown");
    soc_name_len = read(fd, name, size - 1);
    if (soc_name_len > 0)
    {
        name[soc_name_len] = '\0';
        /* replacing the termination character to space */
        /* compatible 是多个 '\0' 分隔的字符串，逐个把中间的 '\0' 改成空格，
         * 才能作为单个 C 字符串整体打印/返回 */
        for (char *ptr = name;; ptr = name)
        {
            ptr += strnlen(name, size);
            if (ptr >= name + soc_name_len - 1)
                break;
            *ptr = ' ';
        }

        printf("chip name: %s\n", name);
    }
    else
    {
        printf("read failed %d\n", soc_name_len);
    }

    close(fd);

    return strdup(name);
}

char *get_kernel_version(char *buf)
{
    char *info = NULL;
    char *ret;
    int size;

    /* buf 为空时自己读 /proc/version（拿到的是堆内存，用完要释放） */
    if (!buf)
        info = get_system_version();
    else
        info = buf;

    /* 跳过固定前缀 "Linux version "，取其后到第一个空格为止的版本号 */
    ret = strdup(info + (sizeof("Linux version ") - 1));
    size = strlen(ret);
    for (int i = 0; i < size; i++)
    {
        char c = *(ret + i);
        if (c == ' ')
        {
            *(ret + i) = '\0';
            break;
        }
    }

    if (!buf)
        free(info);

    return ret;
}

char *get_system_version(void)
{
    const char *path = "/proc/version";
    int fd = open(path, O_RDONLY);
    char info[1024];
    int size = sizeof(info);
    ssize_t len = 0;

    if (fd < 0)
    {
        printf("open %s error\n", path);
        return NULL;
    }

    /* 先填 "unknown" 兜底；/proc/version 只有一行，一次 read 即可读完 */
    snprintf(info, size - 1, "unknown");
    len = read(fd, info, size - 1);
    if (len <= 0)
    {
        printf("read failed %d\n", len);
        return NULL;
    }
    info[len] = '\0';

    close(fd);

    return strdup(info);
}

