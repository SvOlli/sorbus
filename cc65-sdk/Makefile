
include Makefile.cfg

INCLUDES       ?= jam.inc jam_bios.inc fb32x32.inc
SRCS           ?= $(wildcard examples/*.c owncode/*.c owncode/*.s)

targets        :=

define sx4build
my_dest := $$(EXPORT_DIR)/$$(notdir $$(basename $(1))).sx4
my_src  := $(1)

targets += $$(my_dest)
$$(my_dest): $$(my_src) $$(EXPORT_DIR) $$(SORBUS_LIB) includes
	$$(CL65) $$(SYS_FLAGS) $$(CC65_FLAGS) -o $$@ $$< $$(SORBUS_LIB) -l $$@.lst -Ln $$@.labels
endef

.PHONY: all clean includes sx4

all: sx4

$(EXPORT_DIR):
	$(MKDIR) $@

includes $(wildcard lib/*.s) $(wildcard lib/*.c): $(addprefix $(INCLUDE_DIR)/,$(INCLUDES))

$(SORBUS_LIB): $(wildcard lib/*.s) $(wildcard lib/*.c)
	$(CP) $(CC65_BASE_DIR)/lib/supervision.lib $@
	$(AR65) d $@ supervision-stdjoy-joy.o
	for i in $^; do \
	   echo "building $${i}" ; \
	   $(CL65) $(SYS_FLAGS) --asm-include-dir $(INCLUDE_DIR) -I $(INCLUDE_DIR) -c $${i} && \
	   $(AR65) r $@ $${i%.?}.o ; \
	done || $(RM) -v $@

$(foreach target,$(SRCS),$(eval $(call sx4build,$(target))))
ifneq ($(SORBUS_65C02_DIR),)
$(addprefix $(INCLUDE_DIR)/,$(INCLUDES)): $(addprefix $(SORBUS_65C02_DIR)/jam/,$(INCLUDES))
	$(MKDIR) -p $(INCLUDE_DIR)/
	$(CP) $^ $(INCLUDE_DIR)/
endif

sx4: $(targets)

clean:
	$(RM) */*.o $(SORBUS_LIB) $(targets)

dist:	sx4
	$(RM) */*.o
