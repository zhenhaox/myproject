/**
 * @file bt_scan.c
 * @brief 蓝牙设备扫描模块实现（BlueZ D-Bus / GDBus）
 * @note  扫描在独立 pthread 中运行（约 12s），通过 GDBus 订阅
 *        org.bluez 的 InterfacesAdded 信号收集设备；
 *        经典蓝牙与 BLE 设备统一以 org.bluez.Device1 形式上报。
 *        本模块不触碰 LVGL，结果经 mutex 保护的快照接口输出。
 */

#include "bt_scan.h"

#include <gio/gio.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define BT_SCAN_DURATION_US (12 * G_TIME_SPAN_SECOND)  /**< 单次扫描时长 12s */
#define BT_SCAN_LOOP_US     (100 * 1000)               /**< 事件循环粒度 100ms */

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER; /**< 保护以下共享状态 */
static bt_device_t g_devices[BT_SCAN_MAX_DEVICES] = {0};
static int g_device_count = 0;
static bool g_running = false;    /**< 扫描线程存活标志 */
static bool g_stop_req = false;   /**< 外部请求停止标志 */
static char g_error[128] = {0};   /**< 最近一次扫描的错误信息，空串表示无错 */

static pthread_t g_tid;           /**< 扫描线程句柄 */
static bool g_tid_valid = false;  /**< g_tid 是否有效（待 join） */

/**
 * @brief 记录错误信息（静态缓冲，覆盖式）
 */
static void bt_scan_set_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    pthread_mutex_lock(&g_lock);
    vsnprintf(g_error, sizeof(g_error), fmt, ap);
    pthread_mutex_unlock(&g_lock);
    va_end(ap);
}

/**
 * @brief 按地址去重地加入设备；已存在则仅更新 RSSI
 * @note 数组满时静默丢弃后续设备
 */
static void bt_scan_add_device(const char *addr, const char *name, int16_t rssi)
{
    pthread_mutex_lock(&g_lock);
    int i;
    for (i = 0; i < g_device_count; i++) {
        if (strncmp(g_devices[i].addr, addr, sizeof(g_devices[i].addr)) == 0) {
            g_devices[i].rssi = rssi;
            pthread_mutex_unlock(&g_lock);
            return;
        }
    }
    if (g_device_count < BT_SCAN_MAX_DEVICES) {
        snprintf(g_devices[i].addr, sizeof(g_devices[i].addr), "%s", addr);
        snprintf(g_devices[i].name, sizeof(g_devices[i].name), "%s", name);
        g_devices[i].rssi = rssi;
        g_device_count++;
    }
    pthread_mutex_unlock(&g_lock);
}

/**
 * @brief 从 ObjectManager 的接口字典 (a{sa{sv}}) 中提取 Device1 信息并入库
 * @param ifaces 接口字典迭代器，条目为 {接口名, 属性字典}
 * @param adapter_path 若条目含 Adapter1 接口，记录其对象路径到该指针（可为 NULL）
 * @param obj_path 当前条目的 D-Bus 对象路径
 */
static void bt_scan_handle_ifaces(GVariantIter *ifaces, const char *obj_path, char *adapter_path, size_t adapter_path_len)
{
    gchar *iface = NULL;
    GVariantIter *props = NULL;

    while (g_variant_iter_next(ifaces, "{sa{sv}}", &iface, &props)) {
        if (adapter_path != NULL && g_strcmp0(iface, "org.bluez.Adapter1") == 0) {
            g_strlcpy(adapter_path, obj_path, adapter_path_len);
        }
        if (g_strcmp0(iface, "org.bluez.Device1") == 0) {
            char addr[18] = {0};
            char name[64] = "Unknown";
            int16_t rssi = 0;
            gchar *key = NULL;
            GVariant *val = NULL;

            while (g_variant_iter_next(props, "{sv}", &key, &val)) {
                if (g_strcmp0(key, "Address") == 0) {
                    g_strlcpy(addr, g_variant_get_string(val, NULL), sizeof(addr));
                } else if (g_strcmp0(key, "Name") == 0) {
                    g_strlcpy(name, g_variant_get_string(val, NULL), sizeof(name));
                } else if (g_strcmp0(key, "Alias") == 0) {
                    /* Name 优先，Alias 兜底 */
                    if (g_strcmp0(name, "Unknown") == 0) {
                        g_strlcpy(name, g_variant_get_string(val, NULL), sizeof(name));
                    }
                } else if (g_strcmp0(key, "RSSI") == 0) {
                    rssi = (int16_t)g_variant_get_int16(val);
                }
                g_variant_unref(val);
                g_free(key);
            }
            if (addr[0] != '\0') {
                bt_scan_add_device(addr, name, rssi);
            }
        }
        g_variant_iter_free(props);
        g_free(iface);
    }
}

/**
 * @brief InterfacesAdded 信号回调：新设备（或新属性）出现时由 BlueZ 推送
 */
static void bt_scan_on_interfaces_added(GDBusConnection *conn,
                                        const gchar *sender_name,
                                        const gchar *object_path,
                                        const gchar *interface_name,
                                        const gchar *signal_name,
                                        GVariant *parameters,
                                        gpointer user_data)
{
    (void)conn; (void)sender_name; (void)object_path;
    (void)interface_name; (void)signal_name; (void)user_data;

    gchar *dev_path = NULL;
    GVariantIter *ifaces = NULL;
    /* D-Bus 类型签名 (oa{sa{sv}})：o=对象路径，a{sa{sv}}=“接口名→属性字典”的数组 */
    g_variant_get(parameters, "(oa{sa{sv}})", &dev_path, &ifaces);
    bt_scan_handle_ifaces(ifaces, dev_path, NULL, 0);
    g_variant_iter_free(ifaces);
    g_free(dev_path);
}

/**
 * @brief 查询是否收到停止请求
 */
static bool bt_scan_stop_requested(void)
{
    pthread_mutex_lock(&g_lock);
    bool stop = g_stop_req;
    pthread_mutex_unlock(&g_lock);
    return stop;
}

/**
 * @brief 枚举 GetManagedObjects：定位蓝牙适配器，并收集 BlueZ 已知设备
 * @return true=成功拿到管理对象（适配器未找到时 adapter_path 保持空串）
 */
static bool bt_scan_enumerate_managed(GDBusConnection *conn, char *adapter_path, size_t adapter_path_len)
{
    GError *err = NULL;
    GVariant *reply = g_dbus_connection_call_sync(conn,
                                                  "org.bluez", "/",
                                                  "org.freedesktop.DBus.ObjectManager",
                                                  "GetManagedObjects",
                                                  NULL, NULL,
                                                  G_DBUS_CALL_FLAGS_NONE, 3000, NULL, &err);
    if (reply == NULL) {
        bt_scan_set_error("查询蓝牙对象失败: %s", err ? err->message : "unknown");
        g_clear_error(&err);
        return false;
    }

    GVariantIter *objects = NULL;
    g_variant_get(reply, "(a{oa{sa{sv}}})", &objects);

    gchar *obj_path = NULL;
    GVariantIter *ifaces = NULL;
    while (g_variant_iter_next(objects, "{oa{sa{sv}}}", &obj_path, &ifaces)) {
        bt_scan_handle_ifaces(ifaces, obj_path, adapter_path, adapter_path_len);
        g_variant_iter_free(ifaces);
        g_free(obj_path);
    }
    g_variant_iter_free(objects);
    g_variant_unref(reply);
    return true;
}

/**
 * @brief 调用适配器方法（容错，失败仅返回 false 不记错误）
 */
static bool bt_scan_adapter_call(GDBusConnection *conn, const char *adapter_path,
                                 const char *interface, const char *method, GVariant *params)
{
    GError *err = NULL;
    GVariant *reply = g_dbus_connection_call_sync(conn,
                                                  "org.bluez", adapter_path,
                                                  interface, method, params, NULL,
                                                  G_DBUS_CALL_FLAGS_NONE, 5000, NULL, &err);
    if (reply == NULL) {
        g_clear_error(&err);
        return false;
    }
    g_variant_unref(reply);
    return true;
}

/**
 * @brief 扫描工作线程主体
 */
static void *bt_scan_thread(void *arg)
{
    (void)arg;

    /* 线程独立主上下文，GDBus 信号源挂到该上下文，避免与主线程混淆 */
    GMainContext *ctx = g_main_context_new();
    g_main_context_push_thread_default(ctx);

    GError *err = NULL;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
    if (conn == NULL) {
        bt_scan_set_error("D-Bus 连接失败（bluetoothd 未运行？）: %s", err ? err->message : "unknown");
        g_clear_error(&err);
        goto out_ctx;
    }

    char adapter_path[64] = {0};
    if (!bt_scan_enumerate_managed(conn, adapter_path, sizeof(adapter_path))) {
        goto out_conn;
    }
    if (adapter_path[0] == '\0') {
        /* 未枚举到适配器时回退默认路径，由 StartDiscovery 报错兜底 */
        g_strlcpy(adapter_path, "/org/bluez/hci0", sizeof(adapter_path));
    }

    /* 确保适配器已上电，失败不视为致命 */
    bt_scan_adapter_call(conn, adapter_path, "org.freedesktop.DBus.Properties", "Set",
                         g_variant_new("(ssv)", "org.bluez.Adapter1", "Powered",
                                       g_variant_new_boolean(TRUE)));

    guint sub_id = g_dbus_connection_signal_subscribe(conn,
                                                      "org.bluez",
                                                      "org.freedesktop.DBus.ObjectManager",
                                                      "InterfacesAdded",
                                                      "/", NULL,
                                                      G_DBUS_SIGNAL_FLAGS_NONE,
                                                      bt_scan_on_interfaces_added,
                                                      NULL, NULL);

    if (!bt_scan_adapter_call(conn, adapter_path, "org.bluez.Adapter1", "StartDiscovery", NULL)) {
        bt_scan_set_error("启动扫描失败（适配器 %s 不可用？）", adapter_path);
        g_dbus_connection_signal_unsubscribe(conn, sub_id);
        goto out_conn;
    }

    gint64 deadline = g_get_monotonic_time() + BT_SCAN_DURATION_US;
    while (!bt_scan_stop_requested() && g_get_monotonic_time() < deadline) {
        /* 排空已就绪事件后短暂休眠，避免忙轮询 */
        while (g_main_context_iteration(ctx, FALSE)) {
        }
        g_usleep(BT_SCAN_LOOP_US);
    }

    bt_scan_adapter_call(conn, adapter_path, "org.bluez.Adapter1", "StopDiscovery", NULL);
    g_dbus_connection_signal_unsubscribe(conn, sub_id);

out_conn:
    g_object_unref(conn);
out_ctx:
    g_main_context_pop_thread_default(ctx);
    g_main_context_unref(ctx);

    pthread_mutex_lock(&g_lock);
    g_running = false;
    pthread_mutex_unlock(&g_lock);
    return NULL;
}

int bt_scan_start(void)
{
    pthread_mutex_lock(&g_lock);
    if (g_running) {
        pthread_mutex_unlock(&g_lock);
        return -1;
    }
    g_device_count = 0;    /* 清空上轮结果与错误，UI 侧从空列表重新开始轮询 */
    g_error[0] = '\0';
    g_stop_req = false;
    g_running = true;
    pthread_mutex_unlock(&g_lock);

    /* 回收上一次已结束的线程句柄：pthread 不 join 会残留“僵尸”线程资源，
     * 且必须在持锁区外 join，避免与线程内取锁顺序相反造成死锁 */
    if (g_tid_valid) {
        pthread_join(g_tid, NULL);
        g_tid_valid = false;
    }

    if (pthread_create(&g_tid, NULL, bt_scan_thread, NULL) != 0) {
        pthread_mutex_lock(&g_lock);
        g_running = false;
        pthread_mutex_unlock(&g_lock);
        bt_scan_set_error("创建扫描线程失败");
        return -1;
    }
    g_tid_valid = true;
    return 0;
}

void bt_scan_stop(void)
{
    pthread_mutex_lock(&g_lock);
    g_stop_req = true;
    bool valid = g_tid_valid;
    pthread_mutex_unlock(&g_lock);

    if (valid) {
        /* pthread_join 阻塞直到扫描线程真正退出（≈ RTOS 里等任务删除完成），
         * 保证本函数返回后线程不再访问任何共享数据 */
        pthread_join(g_tid, NULL);
        g_tid_valid = false;
    }
}

bool bt_scan_is_running(void)
{
    pthread_mutex_lock(&g_lock);
    bool running = g_running;
    pthread_mutex_unlock(&g_lock);
    return running;
}

int bt_scan_get_devices(bt_device_t *out, int max)
{
    if (out == NULL || max <= 0) {
        return 0;
    }
    /* LVGL 非线程安全：UI 线程（lv_timer 回调）只在持锁期间做整段内存拷贝，
     * 拿到快照后再刷新列表，锁内不做任何 LVGL 调用 */
    pthread_mutex_lock(&g_lock);
    int n = (g_device_count < max) ? g_device_count : max;
    memcpy(out, g_devices, (size_t)n * sizeof(bt_device_t));
    pthread_mutex_unlock(&g_lock);
    return n;
}

const char *bt_scan_get_error(void)
{
    /* 拷贝到独立静态缓冲再返回：锁外使用也不受扫描线程改 g_error 影响 */
    static char err_buf[128];
    pthread_mutex_lock(&g_lock);
    snprintf(err_buf, sizeof(err_buf), "%s", g_error);
    pthread_mutex_unlock(&g_lock);
    return (err_buf[0] != '\0') ? err_buf : NULL;
}
