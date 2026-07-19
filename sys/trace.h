/**
 * @file trace.h
 * @brief ftrace trace_marker 打点接口（atrace 风格），用于内核级耗时/性能分析
 * @note  原理：往 /sys/kernel/debug/tracing/trace_marker 写入文本记录，
 *        内核 ftrace 把它混入调度/中断等事件流，之后可用
 *        cat /sys/kernel/debug/tracing/trace 或 perfetto 图形化查看；
 *        需要内核开启 ftrace 且已挂载 debugfs
 */

#ifndef __TRACE_H__
#define __TRACE_H__

#include <stdint.h>

#if defined(__cplusplus)
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

__BEGIN_DECLS

/**
 * @brief 标记一个代码段开始（ftrace 记录 "B|pid|name"）
 * @param name 代码段名称，与 atrace_end_body 配对成区间
 */
void atrace_begin_body(const char *name);

/**
 * @brief 标记当前代码段结束（ftrace 记录 "E|pid"）
 * @note  ftrace 按栈式配对：一个 E 结束最近未配对的 B
 */
void atrace_end_body();

/**
 * @brief 标记异步事件开始（ftrace 记录 "S|pid|name|cookie"）
 * @param cookie 配对标识，开始/结束须传相同值（异步事件可跨函数/线程）
 */
void atrace_async_begin_body(const char *name, int32_t cookie);

/**
 * @brief 标记异步事件结束（ftrace 记录 "F|pid|name|cookie"）
 * @param cookie 与 atrace_async_begin_body 相同的配对标识
 */
void atrace_async_end_body(const char *name, int32_t cookie);

/**
 * @brief 记录一个整型计数器瞬时值（ftrace 记录 "C|pid|name|value"）
 * @note  在 trace 图上表现为随时间变化的数值曲线，适合统计队列深度等
 */
void atrace_int_body(const char *name, int32_t value);

/**
 * @brief 同 atrace_int_body，数值为 64 位整型
 */
void atrace_int64_body(const char *name, int64_t value);

__END_DECLS

#endif
