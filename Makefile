# 包装 Makefile：在根目录直接执行 make 即可，产物输出到 build/
.PHONY: all clean distclean

all:
	cmake -B build -S .
	$(MAKE) -C build

clean:
	$(MAKE) -C build clean

distclean:
	rm -rf build
