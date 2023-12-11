
$(info This Makefile is not required and for convenience only)
ifeq ($(PICO_SDK_PATH),)
PICO_SDK_PATH=$(shell readlink -f ../pico-sdk)
$(info Using local pico sdk at: $(PICO_SDK_PATH))
else
$(info Using global pico sdk at: $(PICO_SDK_PATH))
endif
PICO_SDK_PATH_CMAKE ?= -DPICO_SDK_PATH=$(PICO_SDK_PATH)
#EXTRA_CMAKE_ARGS += -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON

RELEASE_ARCHIVE := SorbusComputerCores.zip

PICO_SDK_URL ?= https://github.com/raspberrypi/pico-sdk.git
BUILD_DIR ?= $(CURDIR)/build
SRC_DIR := $(CURDIR)/src
JOBS ?= 4

.PHONY: all clean distclean release setup-apt

all: $(PICO_SDK_PATH)/README.md
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) $(PICO_SDK_PATH_CMAKE) $(EXTRA_CMAKE_ARGS)
	rm -f $(BUILD_DIR)/rp2040/native_cpmfs.img # stupid workaround
	make -C $(BUILD_DIR) -j$(JOBS) && echo "\nbuild was successful\n"

clean:
	make -C $(BUILD_DIR) clean
	rm -f $(RELEASE_ARCHIVE)

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

