# 包装 Makefile：在根目录直接执行 make 即可，产物输出到 build/
#
# 本 Makefile 只是 CMake 的一层便捷封装（out-of-source 构建：生成的文件
# 全部放在 build/ 目录，不污染源码目录）：
#   make           配置并编译，产物为 build/myproject
#   make clean     只清编译中间产物，保留 CMake 配置缓存
#   make distclean 连 build/ 目录一起删掉，彻底回到源码初始状态
.PHONY: all clean distclean

all:
	cmake -B build -S .
	$(MAKE) -C build

clean:
	$(MAKE) -C build clean

distclean:
	rm -rf build
