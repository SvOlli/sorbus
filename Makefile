
# Parameters
CPM65_PATH = ../cpm65
#EXTRA_CMAKE_ARGS += -DCMAKE_VERBOSE_MAKEFILE=ON


$(info This Makefile is not required and for convenience only)
ifeq ($(PICO_SDK_PATH),)
PICO_SDK_PATH=$(shell readlink -f ../pico-sdk)
$(info Using local pico sdk at: $(PICO_SDK_PATH))
else
$(info Using global pico sdk at: $(PICO_SDK_PATH))
endif
EXTRA_CMAKE_ARGS += -DPICO_SDK_PATH=$(PICO_SDK_PATH)

CMAKE_CPM65_PATH = $(realpath $(CPM65_PATH))
ifneq ($(CMAKE_CPM65_PATH),)
EXTRA_CMAKE_ARGS += -DCPM65_PATH=$(CMAKE_CPM65_PATH)
endif


RELEASE_ARCHIVE := SorbusComputerCores.zip

PICO_SDK_URL ?= https://github.com/raspberrypi/pico-sdk.git
BUILD_DIR ?= $(CURDIR)/build
SRC_DIR := $(CURDIR)/src
JOBS ?= 4

.PHONY: all clean distclean release setup-apt

all: $(PICO_SDK_PATH)/README.md
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) $(PICO_SDK_PATH_CMAKE) $(EXTRA_CMAKE_ARGS)
	make -C $(BUILD_DIR) -j$(JOBS) && echo "\nbuild was successful\n"

log: $(PICO_SDK_PATH)/README.md
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) -DCMAKE_VERBOSE_MAKEFILE=ON $(PICO_SDK_PATH_CMAKE) $(EXTRA_CMAKE_ARGS) 2>&1 | tee cmake.log
	make -C $(BUILD_DIR) -j$(JOBS) 2>&1 | tee make.log

clean:
	make -C $(BUILD_DIR) clean
	rm -f $(RELEASE_ARCHIVE) cmake.log make.log

distclean:
	rm -rf $(RELEASE_ARCHIVE) $(BUILD_DIR)

$(PICO_SDK_PATH)/README.md:
	mkdir -p $(PICO_SDK_PATH)
	git clone --depth 1 --recurse-submodules --shallow-submodules $(PICO_SDK_URL) $(PICO_SDK_PATH)

setup-apt:
	sudo apt update
	sudo apt install gdb-multiarch cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib cc65 microcom p7zip-full cpmtools

$(RELEASE_ARCHIVE): all
	for i in $$(ls -1 $(BUILD_DIR)/rp2040/*.uf2|grep -v _test.uf2$$); do cp -v $${i} sorbus-computer-$$(basename $${i});done
	cp doc/README_release.txt README.txt
	rm -f $@
	7z a -mx=9 -bd -sdel $@ README.txt *.uf2

release: $(RELEASE_ARCHIVE)

