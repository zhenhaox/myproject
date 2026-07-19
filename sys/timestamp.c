/**
 * @file timestamp.c
 * @brief 毫秒/微秒时间戳与简易打点计时实现
 * @note  gettimeofday 取的是墙上时钟，会被 NTP 校时/手动改时间影响；
 *        本项目只用它做日志打印和粗粒度耗时统计，精度足够。
 *        若需要不受校时影响的单调计时，应改用 clock_gettime(CLOCK_MONOTONIC)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

uint32_t clock_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    /* uint32_t 溢出后自然回绕，做差值计时不受影响 */
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

uint64_t clock_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000 + tv.tv_usec);
}

void timestamp(char *fmt, int index, int start)
{
    /* 128 个计时槽：static 驻留全局区，跨调用保持起点（多线程同时用会互相覆盖） */
    static long int st[128];
    struct timeval t;

    if (start)
    {
        printf("[%d %s]", index, fmt);
        gettimeofday(&t, NULL);
        st[index] = t.tv_sec * 1000000 + t.tv_usec;
    }
    else
    {
        /* 终点：打印与同一 index 起点的微秒差值 */
        gettimeofday(&t, NULL);
        printf("[%d # cost %ld us]\n", index,
               (t.tv_sec * 1000000 + t.tv_usec) - st[index]);
    }
}

