
CC     ?= gcc
CFLAGS += -Os
EXE    ?=

targets :=

define sorbus_tool
my_dest := $(basename $(1))$(EXE)
my_src  := $(1)

targets += $$(my_dest)
$$(my_dest): $$(my_src)
	$$(CC) $$(CFLAGS) -o $$@ $$^ $$(LDFLAGS) $$(LDFLAGS_$(basename $(1)))
endef


.PHONY: all clean targets

all: targets

$(foreach tool,$(wildcard *.c),$(eval $(call sorbus_tool,$(tool))))

targets: $(targets)

clean:
	rm -f $(targets)
