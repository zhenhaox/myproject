/**
 * @file cpu.c
 * @brief 整机 CPU 占用率统计：对比两次读取的 /proc/stat 累计值，按差值算使用率
 * @note  /proc/stat 是内核导出的只读伪文件（≈ 外设寄存器的文件化接口），
 *        其中 "cpu" 首行记录自开机以来各状态的累计 tick；
 *        本实现用 static 变量保存上次采样，每次调用返回一个统计窗口的占用率
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** /proc/stat 首行（cpu 总计行）的解析结果，各字段单位均为内核 tick（USER_HZ，通常 10ms） */
typedef struct cpu_occupy_
{
    char name[20];          /**< 行首标签，总计行固定为 "cpu" */
    unsigned int user;      /**< 用户态累计 tick */
    unsigned int nice;      /**< 低优先级（nice）用户态累计 tick */
    unsigned int system;    /**< 内核态累计 tick */
    unsigned int idle;      /**< 空闲累计 tick，计算占用率的关键项 */
    unsigned int iowait;    /**< 等待 IO 累计 tick */
    unsigned int irq;       /**< 硬中断累计 tick */
    unsigned int softirq;   /**< 软中断累计 tick */
} cpu_occupy_t;

/**
 * @brief 由两次采样的差值计算 CPU 占用率
 * @param o 上一次采样
 * @param n 本次采样
 * @return 占用率百分比（0~100）；总 tick 无增量时返回 0（防除零）
 * @note   公式：100 - Δidle / Δtotal * 100。/proc/stat 各项是开机累计值，
 *         必须做差才能得到两次调用之间这段时间的占用率
 */
static double cal_cpuoccupy(cpu_occupy_t *o, cpu_occupy_t *n)
{
    double od, nd;
    double id, sd;
    double cpu_use;

    od = (double)(o->user + o->nice + o->system + o->idle + o->softirq + o->iowait +
                  o->irq);
    nd = (double)(n->user + n->nice + n->system + n->idle + n->softirq + n->iowait +
                  n->irq);

    id = (double)(n->idle);
    sd = (double)(o->idle);
    if ((nd - od) != 0)
        cpu_use = 100.0 - ((id - sd)) / (nd - od) * 100.00;
    else
        cpu_use = 0;
    return cpu_use;
}

/**
 * @brief 读取 /proc/stat 首行（cpu 总计行）并解析到结构体
 * @param cpust 输出结构体；读取失败时保持原内容不变
 * @note  只关心首行总计数据，fgets 读一行即够
 */
static void get_cpuoccupy(cpu_occupy_t *cpust)
{
    FILE *fd;
    int n;
    char buff[256];
    cpu_occupy_t *cpu_occupy;
    cpu_occupy = cpust;

    /* procfs 伪文件不支持随机访问，按普通文本文件顺序读即可 */
    fd = fopen("/proc/stat", "r");
    if (fd == NULL)
    {
        printf("open /proc/stat failed\n");
        return;
    }
    if (fgets(buff, sizeof(buff), fd) == NULL)
        printf("fgets failed\n");

    sscanf(buff, "%s %u %u %u %u %u %u %u", cpu_occupy->name,
           &cpu_occupy->user, &cpu_occupy->nice, &cpu_occupy->system,
           &cpu_occupy->idle, &cpu_occupy->iowait, &cpu_occupy->irq,
           &cpu_occupy->softirq);

    fclose(fd);
}

double get_cpu_usage(void)
{
    /* cpu_stat1 保存上次采样：static 驻留全局区，跨调用保持（也因此本函数不可重入） */
    static cpu_occupy_t cpu_stat1;
    cpu_occupy_t cpu_stat2;
    double cpu;

    /* 本次采样与上次采样做差，随后把本次存为下次的基准 */
    get_cpuoccupy(&cpu_stat2);
    cpu = cal_cpuoccupy(&cpu_stat1, &cpu_stat2);
    memcpy(&cpu_stat1, &cpu_stat2, sizeof(cpu_occupy_t));

    return cpu;
}

