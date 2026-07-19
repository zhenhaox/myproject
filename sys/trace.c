/**
 * @file trace.c
 * @brief ftrace trace_marker 打点实现（atrace 风格，源自 Android 的 atrace）
 * @note  通过 TRACE_ON 宏开关：定义后打点真正写入内核 ftrace 缓冲区；
 *        注释掉 TRACE_ON 则全部编译为空函数，发布版本可零成本关闭。
 *        使用前提：内核开启 ftrace，且已挂载 debugfs
 *        （mount -t debugfs none /sys/kernel/debug）
 */

#include "trace.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>

#define TRACE_ON

#ifdef TRACE_ON

/* 分支预测提示：让编译器把“大概率成立”的分支排在前面，打点路径尽量低开销 */
#ifdef __cplusplus
#define CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#else
#define CC_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#endif

#define ATRACE_MESSAGE_LENGTH 1024  /**< 单条打点消息的最大长度（ftrace 侧同样有限制） */

static int atrace_marker_fd     = -1;  /**< trace_marker 节点句柄，-1 表示尚未打开 */
static int init_ok = 0;                /**< 是否已尝试过初始化（只尝试一次） */

/**
 * @brief 打开 trace_marker 节点（惰性初始化，仅首次打点时执行）
 * @return 0=可用，-1=打开失败
 * @note  打开失败也把 init_ok 置 1：不再重试，后续 write 失败仅打印警告，
 *        保证打点问题绝不影响业务逻辑（性能调试工具不应拖垮程序）
 */
static int trace_init_once(void)
{
    if (init_ok == 0)
    {
        /* trace_marker 是内核 ftrace 的用户态写入口（只写伪文件） */
        atrace_marker_fd = open("/sys/kernel/debug/tracing/trace_marker",
                                O_WRONLY | O_CLOEXEC);
        if (atrace_marker_fd == -1)
        {
            printf("Error opening trace file: %s (%d)", strerror(errno), errno);
            return -1;
        }
    }

    init_ok = 1;
    return 0;
}

/*
 * 打点核心宏：按 ftrace 文本协议拼一条消息并写入 trace_marker。
 * 消息格式：B=区间开始，E=区间结束，S/F=异步事件开始/结束，C=计数器瞬时值。
 * name 过长导致消息超出 buf 时，截断 name 重写一次，保证 len 与缓冲区一致。
 */
#define WRITE_MSG(format_begin, format_end, name, value) { \
    char buf[ATRACE_MESSAGE_LENGTH]; \
    if (CC_UNLIKELY(!init_ok)) \
        trace_init_once(); \
    int pid = getpid(); \
    int len = snprintf(buf, sizeof(buf), format_begin "%s" format_end, pid, \
        name, value); \
    if (len >= (int) sizeof(buf)) { \
        /* Given the sizeof(buf), and all of the current format buffers, \
         * it is impossible for name_len to be < 0 if len >= sizeof(buf). */ \
        int name_len = strlen(name) - (len - sizeof(buf)) - 1; \
        /* Truncate the name to make the message fit. */ \
        printf("Truncated name in %s: %s\n", __FUNCTION__, name); \
        len = snprintf(buf, sizeof(buf), format_begin "%.*s" format_end, pid, \
            name_len, name, value); \
    } \
    if (write(atrace_marker_fd, buf, len) != len) \
        printf("Warning: write failed\n"); \
}

void atrace_begin_body(const char *name)
{
    WRITE_MSG("B|%d|", "%s", name, "");
}

void atrace_end_body()
{
    WRITE_MSG("E|%d", "%s", "", "");
}

void atrace_async_begin_body(const char *name, int32_t cookie)
{
    WRITE_MSG("S|%d|", "|%" PRId32, name, cookie);
}

void atrace_async_end_body(const char *name, int32_t cookie)
{
    WRITE_MSG("F|%d|", "|%" PRId32, name, cookie);
}

void atrace_int_body(const char *name, int32_t value)
{
    WRITE_MSG("C|%d|", "|%" PRId32, name, value);
}

void atrace_int64_body(const char *name, int64_t value)
{
    WRITE_MSG("C|%d|", "|%" PRId64, name, value);
}

#else

/* TRACE_ON 未定义时的空实现：调用处无需改动，编译器会直接优化掉这些空函数 */

void atrace_begin_body(__attribute__((unused)) const char *name)
{
}

void atrace_end_body()
{
}

void atrace_async_begin_body(__attribute__((unused)) const char *name,
                             __attribute__((unused)) int32_t cookie)
{
}

void atrace_async_end_body(__attribute__((unused)) const char *name,
                           __attribute__((unused)) int32_t cookie)
{
}

void atrace_int_body(__attribute__((unused)) const char *name,
                     __attribute__((unused)) int32_t value)
{
}

void atrace_int64_body(__attribute__((unused)) const char *name,
                       __attribute__((unused)) int64_t value)
{
}

#endif
