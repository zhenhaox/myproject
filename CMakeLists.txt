#@author: Deng Zhimao
#@email: dengzhimao@alientek.com
#B站作品: https://space.bilibili.com/474103963?spm_id_from=333.788.0.0

#开源电子网: http://www.openedv.com/forum.php
#正点原子官网: https://www.alientek.com
#店铺: https://zhengdianyuanzi.tmall.com

#LICENCE GPLV3 
#
# 本文件是 LVGL 演示程序的交叉编译配置：
# 在 PC（x86 Ubuntu）上用 aarch64 工具链编译出开发板可运行的 myproject。
# 常规用法（见根目录 Makefile）：cmake -B build -S . && make -C build
# 生成的可执行文件与开机启动脚本 S11demo_lvgl 会被打包进 Buildroot 根文件系统。
cmake_minimum_required(VERSION 3.8)

project(myproject)

# 交叉编译工具链配置。
# 两种编译方式：
#   1) 单独在 PC 上编译（默认）：使用正点原子提供的独立工具链，需指定交叉编译器和 sysroot；
#   2) 随 Buildroot SDK 整体编译：定义 BUILD_WITH_BUILDROOT，工具链与 sysroot 由
#      Buildroot 外部包机制自动注入，这里的配置全部跳过，两者不能混用。
# sysroot ≈ 开发板根文件系统的“副本”，交叉编译器从这里找头文件和库（而不是用 PC 的）。
if(NOT BUILD_WITH_BUILDROOT)
    set(TOOLCHAIN_DIR /opt/atk-dlrv1126b-toolchain)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-buildroot-linux-gnu-g++)
    set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-buildroot-linux-gnu-gcc)
    set(SYSROOT ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot/usr/include)
    set(CMAKE_SYSROOT ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot)
endif()


# 第三方库头文件路径（都在开发板 sysroot 里）：
# lvgl/lv_drivers 为 GUI 库；c++/13.3.0 为 C++ 标准库头文件；
# glib-2.0 及其 platform 头文件目录（glibconfig.h 在 lib 下）供蓝牙扫描的 GDBus 使用。
# 注意：glib-2.0 的 include 路径同时有 usr/include 和 usr/lib 两处，缺一不可。
include_directories(
        ${CMAKE_SYSROOT}/usr/include/lvgl
        ${CMAKE_SYSROOT}/usr/include/lvgl/lv_drivers
        ${CMAKE_SYSROOT}/usr/include/c++/13.3.0
        ${CMAKE_SYSROOT}/usr/include/glib-2.0
        ${CMAKE_SYSROOT}/usr/lib/glib-2.0/include
        )

# 全局编译宏：选择 LVGL 的底层驱动——显示用 DRM/KMS（/dev/dri），触摸用 evdev（/dev/input/eventX），
# lv_port_disp.c / lv_port_indev.c 里用 #ifdef 按这两个宏编译对应后端。
add_definitions(-DUSE_DRM=1 -DUSE_EVDEV=1)

# 本项目自身的头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/lvgl8)
include_directories(${PROJECT_SOURCE_DIR}/common)
include_directories(${PROJECT_SOURCE_DIR}/sys)
include_directories(${PROJECT_SOURCE_DIR}/user)

# 把各目录下所有源文件收集进 SRCS。
# 注意 aux_source_directory 只收集已存在的文件：新增 .c 文件后需重新执行 cmake 才会被编译。
aux_source_directory(${PROJECT_SOURCE_DIR}/lvgl8 SRCS)
aux_source_directory(${PROJECT_SOURCE_DIR}/sys SRCS)
aux_source_directory(${PROJECT_SOURCE_DIR}/common SRCS)
aux_source_directory(${PROJECT_SOURCE_DIR}/user SRCS)
aux_source_directory(. SRCS)

add_executable(${PROJECT_NAME} ${SRCS})

# 链接的库（来自开发板 sysroot）：
#   lvgl/lv_drivers  GUI 库本体；freetype  运行时加载中文字体；
#   drm              DRM/KMS 显示接口；evdev  触摸输入接口；
#   gio/gobject/glib GLib 全家桶，蓝牙扫描的 GDBus（D-Bus 客户端）依赖它们；
#   pthread          POSIX 线程库（≈ 裸机里的任务），扫描工作线程需要。
target_link_libraries(${PROJECT_NAME}
        lvgl lv_drivers freetype drm evdev
        gio-2.0 gobject-2.0 glib-2.0 pthread
)

# make install 时的安装位置（Buildroot 打包根文件系统用）：
# 可执行程序装到 /usr/bin，启动脚本装到 /etc/init.d（开机自启，见 S11demo_lvgl 注释）。
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(PROGRAMS S11demo_lvgl DESTINATION /etc/init.d)

# 自定义目标：make distclean 删除 CMake 生成的缓存文件，回到干净的源码状态。
# 与根 Makefile 的 distclean（直接 rm -rf build）效果类似，二选一即可。
add_custom_target(distclean
    COMMAND ${CMAKE_COMMAND} -E remove CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove Makefile
    COMMAND ${CMAKE_COMMAND} -E remove_directory CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_NAME}
    COMMENT "Clean all CMake generated files and ${PROJECT_NAME}"
)
