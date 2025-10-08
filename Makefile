
# This Makefile is not required and for convenience only
# you can use the CMake/Pico-SDK project in the src/ folder

# Parameters
CPM65_PATH = $(shell readlink -f ../cpm65)
LLVM_MOS_PATH = $(shell readlink -f ../llvm-mos-sdk/bin)
CC65_SDK_DIR = cc65-sdk

# Commands
INSTALL      = install
MKDIR        = mkdir -p
RM           = rm -rf
LS           = ls -l
GIT_CHECKOUT = git clone --depth 1 --recurse-submodules --shallow-submodules

CC65_SDK_INCLUDES = jam.inc jam_bios.inc fb32x32.inc
CC65_SDK_TOOLS    = wozcat.c timcat.c

$(info # This Makefile is not required and for convenience only)

ifneq ($(LLVM_MOS_PATH),)
export PATH := $(PATH):$(LLVM_MOS_PATH)
endif

ifeq ($(PICO_SDK_PATH),)
PICO_SDK_PATH=$(shell readlink -f ../pico-sdk)
$(info # Using local pico sdk at: $(PICO_SDK_PATH))
else
$(info # Using global pico sdk at: $(PICO_SDK_PATH))
endif
EXTRA_CMAKE_ARGS += -DPICO_SDK_PATH="$(PICO_SDK_PATH)"

ifeq ($(PICO_EXTRAS_PATH),)
PICO_EXTRAS_PATH=$(shell readlink -f ../pico-extras)
$(info # Using local pico extras at: $(PICO_EXTRAS_PATH))
else
$(info # Using global pico extras at: $(PICO_EXTRAS_PATH))
endif
EXTRA_CMAKE_ARGS += -DPICO_EXTRAS_PATH="$(PICO_EXTRAS_PATH)"

CMAKE_CPM65_PATH = $(realpath $(CPM65_PATH))
ifneq ($(CMAKE_CPM65_PATH),)
EXTRA_CMAKE_ARGS += -DCPM65_PATH=$(CMAKE_CPM65_PATH)
endif

# workaround to suppress annoying warning
export PICOTOOL_FETCH_FROM_GIT_PATH ?= $(realpath ../picotool)

RELEASE_ARCHIVE := SorbusComputerCores.zip

PICO_SDK_URL ?= https://github.com/raspberrypi/pico-sdk.git
PICO_EXTRAS_URL ?= https://github.com/raspberrypi/pico-extras.git
BUILD_DIR ?= $(CURDIR)/build
SRC_DIR := $(CURDIR)/src
JOBS ?= 4

cc65_sdk_deps :=

define cc65_sdk_include
my_dest := $$(CC65_SDK_DIR)/include/$(1)
my_src  := src/65c02/jam/$(1)
cc65_sdk_deps += $$(my_dest)
$$(my_dest): $$(my_src)
	$(INSTALL) -D -m0644 $$< $$@
endef

define cc65_sdk_tool
my_dest := $$(CC65_SDK_DIR)/tools/$(1)
my_src  := src/tools/$(1)
cc65_sdk_deps += $$(my_dest)
$$(my_dest): $$(my_src)
	$(INSTALL) -D -m0644 $$< $$@
endef

.PHONY: all clean distclean release setup-apt cc65-sdk

all: $(PICO_SDK_PATH)/README.md $(PICO_EXTRAS_PATH)/README.md
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) $(EXTRA_CMAKE_ARGS)
	make -C $(BUILD_DIR) -j$(JOBS) && echo "\nbuild was successful\n"

log: $(PICO_SDK_PATH)/README.md $(PICO_EXTRAS_PATH)/README.md
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) -DCMAKE_VERBOSE_MAKEFILE=ON $(EXTRA_CMAKE_ARGS) 2>&1 | tee cmake.log
	make -C $(BUILD_DIR) -j$(JOBS) 2>&1 | tee make.log

$(foreach include,$(CC65_SDK_INCLUDES),$(eval $(call cc65_sdk_include,$(include))))
$(foreach tool,$(CC65_SDK_TOOLS),$(eval $(call cc65_sdk_tool,$(tool))))

cc65-sdk: $(cc65_sdk_deps)
	$(MAKE) -C $@ EXPORT_DIR="$(CURDIR)/src/bin/cpm/10"

clean:
	make -C $(BUILD_DIR) clean
	$(RM) $(RELEASE_ARCHIVE) cmake.log make.log $(cc65_sdk_deps)
	make -C $(CC65_SDK_DIR) clean

distclean:
	$(RM) $(RELEASE_ARCHIVE) $(BUILD_DIR) make.log cmake.log $(cc65_sdk_deps)
	make -C $(CC65_SDK_DIR) EXPORT_DIR=../src/bin/cpm/10 clean

$(PICO_SDK_PATH)/README.md:
	$(MKDIR) $(PICO_SDK_PATH)
	$(GIT_CHECKOUT) $(PICO_SDK_URL) $(PICO_SDK_PATH)

$(PICO_EXTRAS_PATH)/README.md:
	$(MKDIR) $(PICO_EXTRAS_PATH)
	$(GIT_CHECKOUT) $(PICO_EXTRAS_URL) $(PICO_EXTRAS_PATH)

picotool: $(PICO_SDK_PATH)/README.md
	src/tools/external-picotool.sh
	sudo cp $(PICOTOOL_FETCH_FROM_GIT_PATH)/udev/99-picotool.rules /etc/udev/rules.d/

paths:
	@echo 'PICO_SDK_PATH="$(PICO_SDK_PATH)"'
	@echo 'PICO_EXTRAS_PATH="$(PICO_EXTRAS_PATH)"'
	@echo 'PICOTOOL_FETCH_FROM_GIT_PATH="$(PICOTOOL_FETCH_FROM_GIT_PATH)"'

# these packages are required to create the release package
setup-apt:
	sudo apt update
	sudo apt install gdb-multiarch cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib cc65 microcom p7zip-full cpmtools build-essential pkg-config libgd-dev

# this is additionally required on a development host
setup-dev: setup-apt
	sudo apt install pkgconf libusb-1.0-0-dev microcom
	sudo gpasswd -a $(USER) dialout
	sudo gpasswd -a $(USER) plugdev

setup-external:
	sudo apt install 64tass libreadline-dev libfmt-dev moreutils fp-compiler ninja-build zip unzip

$(RELEASE_ARCHIVE): $(CC65_SDK_DIR) all
	for i in $$(ls -1 $(BUILD_DIR)/rp2040/*.uf2|grep -v _test.uf2$$); do cp -v $${i} sorbus-computer-$$(basename $${i});done
	cp doc/README_release.txt README.txt
	$(RM) $@
	7z a -mx=9 -bd -sdel $@ README.txt *.uf2
	make -C $(CC65_SDK_DIR) clean
	7z a -mx=9 -bd $@ $(CC65_SDK_DIR) doc/apple1.md doc/monitors.md doc/sysmon.md doc/jam.rst doc/images/WS2812_order.gif

release: sanitycheck $(RELEASE_ARCHIVE)

sanitycheck:
	: src/ should not contain filename with spaces
	[ $$(find src/ -name "* *" | wc -l) -eq 0 ]

