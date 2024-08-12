
# Parameters
#CPM65_PATH = ../cpm65
LLVM_MOS_SDK_VERSION = v18.0.0
#EXTRA_CMAKE_ARGS += -DCMAKE_VERBOSE_MAKEFILE=ON

# Commands
MKDIR = mkdir -p
RM = rm -rf
LS = ls -l
GIT_CHECKOUT = git clone --depth 1 --recurse-submodules --shallow-submodules

$(info This Makefile is not required and for convenience only)

ifeq ($(PICO_SDK_PATH),)
PICO_SDK_PATH=$(shell readlink -f ../pico-sdk)
$(info Using local pico sdk at: $(PICO_SDK_PATH))
else
$(info Using global pico sdk at: $(PICO_SDK_PATH))
endif
EXTRA_CMAKE_ARGS += -DPICO_SDK_PATH="$(PICO_SDK_PATH)"

ifeq ($(PICO_EXTRAS_PATH),)
PICO_EXTRAS_PATH=$(shell readlink -f ../pico-extras)
$(info Using local pico extras at: $(PICO_EXTRAS_PATH))
else
$(info Using global pico extras at: $(PICO_EXTRAS_PATH))
endif
#EXTRA_CMAKE_ARGS += -DPICO_EXTRAS_PATH="$(PICO_EXTRAS_PATH)"

CMAKE_CPM65_PATH = $(realpath $(CPM65_PATH))
ifneq ($(CMAKE_CPM65_PATH),)
EXTRA_CMAKE_ARGS += -DCPM65_PATH=$(CMAKE_CPM65_PATH)
else
  ifneq ($(wildcard external-build/cpm65/.*),)
  EXTRA_CMAKE_ARGS += -DCPM65_PATH="$(realpath external-build/cpm65)"
  endif
endif

# workaround to suppress annoying warning
export PICOTOOL_FETCH_FROM_GIT_PATH ?= $(realpath ../picotool)

RELEASE_ARCHIVE := SorbusComputerCores.zip

PICO_SDK_URL ?= https://github.com/raspberrypi/pico-sdk.git
PICO_EXTRAS_URL ?= https://github.com/raspberrypi/pico-extras.git
BUILD_DIR ?= $(CURDIR)/build
SRC_DIR := $(CURDIR)/src
JOBS ?= 4

.PHONY: all clean distclean release setup-apt

all: $(PICO_SDK_PATH)/README.md
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) $(EXTRA_CMAKE_ARGS)
	make -C $(BUILD_DIR) -j$(JOBS) && echo "\nbuild was successful\n"

log: $(PICO_SDK_PATH)/README.md
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) -DCMAKE_VERBOSE_MAKEFILE=ON $(EXTRA_CMAKE_ARGS) 2>&1 | tee cmake.log
	make -C $(BUILD_DIR) -j$(JOBS) 2>&1 | tee make.log

clean:
	make -C $(BUILD_DIR) clean
	$(RM) $(RELEASE_ARCHIVE) cmake.log make.log

distclean:
	$(RM) $(RELEASE_ARCHIVE) $(BUILD_DIR) make.log cmake.log

$(PICO_SDK_PATH)/README.md:
	$(MKDIR) $(PICO_SDK_PATH)
	$(GIT_CHECKOUT) $(PICO_SDK_URL) $(PICO_SDK_PATH)

$(PICO_EXTRAS_PATH)/README.md:
	$(MKDIR) $(PICO_EXTRAS_PATH)
	$(GIT_CHECKOUT) $(PICO_EXTRAS_URL) $(PICO_EXTRAS_PATH)

picotool: $(PICO_SDK_PATH)/README.md
	src/tools/external-picotool.sh

# these packages are required to create the release package
setup-apt:
	sudo apt update
	sudo apt install gdb-multiarch cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib cc65 microcom p7zip-full cpmtools build-essential

# this is additionally required on a development host
setup-dev: setup-apt
	sudo apt install pkgconf libusb-1.0-0-dev microcom
	sudo gpasswd -a $(USER) dialout
	sudo gpasswd -a $(USER) plugdev
	sudo cp doc/99-picotool.rules /etc/udev/rules.d/

setup-external:
	sudo apt-get install 64tass libreadline-dev libfmt-dev moreutils fp-compiler ninja-build zip unzip

$(RELEASE_ARCHIVE): all
	for i in $$(ls -1 $(BUILD_DIR)/rp2040/*.uf2|grep -v _test.uf2$$); do cp -v $${i} sorbus-computer-$$(basename $${i});done
	cp doc/README_release.txt README.txt
	$(RM) $@
	7z a -mx=9 -bd -sdel $@ README.txt *.uf2

release: $(RELEASE_ARCHIVE)

