
# TODO: only clone Pico SDK when not set globally

$(info This Makefile is not required and for convenience only)
ifeq ($(PICO_SDK_PATH),)
PICO_SDK_PATH=$(shell readlink -f ../pico-sdk)
PICO_SDK_PATH_CMAKE ?= -DPICO_SDK_PATH=$(PICO_SDK_PATH)
$(info Using local pico sdk at: $(PICO_SDK_PATH))
else
$(info Using global pico sdk at: $(PICO_SDK_PATH))
endif

PICO_SDK_URL ?= https://github.com/raspberrypi/pico-sdk.git

all: $(PICO_SDK_PATH)/README.md
	cmake -S $(CURDIR)/src -B $(CURDIR)/build
	make -C $(CURDIR)/build

$(PICO_SDK_PATH)/README.md:
	mkdir -p $(PICO_SDK_PATH)
	git clone --recurse-submodules $(PICO_SDK_URL) $(PICO_SDK_PATH)

setup-apt:
	sudo apt install gdb-multiarch cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib

setup-udev:
	echo -e '#Picoprobe\nSUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", MODE="0666"' | sudo dd of=/etc/udev/rules.d/99-pico.rule
