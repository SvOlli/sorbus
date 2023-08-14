
$(info This Makefile is not required and for convenience only)
ifeq ($(PICO_SDK_PATH),)
PICO_SDK_PATH=$(shell readlink -f ../pico-sdk)
$(info Using local pico sdk at: $(PICO_SDK_PATH))
else
$(info Using global pico sdk at: $(PICO_SDK_PATH))
endif
PICO_SDK_PATH_CMAKE ?= -DPICO_SDK_PATH=$(PICO_SDK_PATH)

RELEASE_ARCHIVE := SorbusComputerCores.zip

PICO_SDK_URL ?= https://github.com/raspberrypi/pico-sdk.git

.PHONY: all clean distclean release setup-apt

all: $(PICO_SDK_PATH)/README.md
	cmake -S $(CURDIR)/src -B $(CURDIR)/build $(PICO_SDK_PATH_CMAKE)
	make -C $(CURDIR)/build

clean:
	make -C $(CURDIR)/build clean
	rm -f $(RELEASE_ARCHIVE)

distclean:
	rm -rf $(RELEASE_ARCHIVE) $(CURDIR)/build

$(PICO_SDK_PATH)/README.md:
	mkdir -p $(PICO_SDK_PATH)
	git clone --recurse-submodules $(PICO_SDK_URL) $(PICO_SDK_PATH)

setup-apt:
	sudo apt install gdb-multiarch cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib cc65 microcom p7zip-full

$(RELEASE_ARCHIVE): all
	for i in $$(ls -1 build/rp2040/*.uf2|grep -v _test.uf2$$); do cp -v $${i} sorbus-computer-$$(basename $${i});done
	cp doc/README_release.txt README.txt
	rm -f $@
	7z a -mx=9 -bd -sdel $@ README.txt *.uf2

release: $(RELEASE_ARCHIVE)

