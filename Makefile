# 项目基础路径
BASE_DIR := $(shell pwd)

# 所有 cc 文件
SOURCES := $(shell find src -name *.cc)

# 所有 头文件
HEADERS := $(shell find include -name *.hh)

# 编译选项
CXXFLAGS := -g -std=c++11 -Iinclude -Iext/libelfin

# 链接选项
ELF_DIR := ${BASE_DIR}/ext/libelfin/elf
DWARF_DIR := ${BASE_DIR}/ext/libelfin/dwarf
CXXLDFLAGS := -L$(ELF_DIR) -L$(DWARF_DIR) -Wl,-rpath,$(ELF_DIR):$(DWARF_DIR)

# 依赖库
LIBRARIES := -lelf++ -ldwarf++

.PHONY: libelfin
libelfin:
	make -C ext/libelfin

bdb: libelfin $(SOURCES) $(HEADERS)
	g++ $(CXXFLAGS) $(CXXLDFLAGS) $< -o $@ $(LIBRARIES)
	make -C tracees


.PHONY: clean
clean:
	make -C tracees clean
	rm -rf bdb
	make -C ext/libelfin clean
