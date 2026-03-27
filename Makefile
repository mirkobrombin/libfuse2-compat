CC ?= cc
AR ?= ar
INSTALL ?= install
PKG_CONFIG ?= pkg-config

PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
DESTDIR ?=

CPPFLAGS ?=
CFLAGS ?= -O2 -g
LDFLAGS ?=

WARNINGS := -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wstrict-prototypes -Wmissing-prototypes -Werror=implicit-function-declaration
COMMON_CFLAGS := -std=c11 -fPIC -fvisibility=hidden -D_GNU_SOURCE $(WARNINGS)
LIBS := -ldl -lpthread

BUILD_DIR := build
TEST_BUILD_DIR := tests/build
REAL_LIB := $(BUILD_DIR)/libfuse.so.2.9.9
SONAME_LINK := $(BUILD_DIR)/libfuse.so.2
DEV_LINK := $(BUILD_DIR)/libfuse.so
OBJECTS := $(BUILD_DIR)/bridge.o $(BUILD_DIR)/fuse3_loader.o

.PHONY: all clean check check-unit check-symbols check-system check-abi \
	install uninstall

all: $(REAL_LIB) $(SONAME_LINK) $(DEV_LINK)

$(BUILD_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CFLAGS) -Isrc -c $< -o $@

$(REAL_LIB): $(OBJECTS) src/libfuse2-compat.map
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libfuse.so.2 \
		-Wl,--version-script,src/libfuse2-compat.map \
		-o $@ $(OBJECTS) $(LIBS)

$(SONAME_LINK): $(REAL_LIB)
	ln -sfn $(notdir $(REAL_LIB)) $@

$(DEV_LINK): $(REAL_LIB)
	ln -sfn $(notdir $(REAL_LIB)) $@

$(TEST_BUILD_DIR)/libfuse3-test.so: tests/fake_fuse3.c src/fuse3_abi.h | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 -fPIC -D_GNU_SOURCE \
		$(filter-out -Wmissing-prototypes,$(WARNINGS)) \
		-Isrc -shared $< -o $@

$(TEST_BUILD_DIR)/test_bridge: tests/test_bridge.c src/fuse2_abi.h $(SONAME_LINK) | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(WARNINGS) -D_GNU_SOURCE -Isrc \
		$< -L$(BUILD_DIR) -Wl,-rpath,'$$ORIGIN/../../build' \
		-l:libfuse.so.2.9.9 -ldl -o $@

$(TEST_BUILD_DIR)/backend_probe: tests/backend_probe.c src/fuse2_abi.h $(SONAME_LINK) | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(WARNINGS) -D_GNU_SOURCE -Isrc \
		$< -L$(BUILD_DIR) -Wl,-rpath,'$$ORIGIN/../../build' \
		-l:libfuse.so.2.9.9 -o $@

check-unit: $(TEST_BUILD_DIR)/libfuse3-test.so $(TEST_BUILD_DIR)/test_bridge
	LIBFUSE2_COMPAT_BACKEND=$(abspath $(TEST_BUILD_DIR)/libfuse3-test.so) \
		$(TEST_BUILD_DIR)/test_bridge

check-symbols: all
	./tests/test_symbols.sh $(REAL_LIB)

check-system: $(TEST_BUILD_DIR)/backend_probe
	env -u LIBFUSE2_COMPAT_BACKEND $(TEST_BUILD_DIR)/backend_probe

check-abi:
	./tests/test_abi_layout.sh

check: check-unit check-symbols check-system check-abi

install: all
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0755 $(REAL_LIB) $(DESTDIR)$(LIBDIR)/libfuse.so.2.9.9
	ln -sfn libfuse.so.2.9.9 $(DESTDIR)$(LIBDIR)/libfuse.so.2

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/libfuse.so.2 $(DESTDIR)$(LIBDIR)/libfuse.so.2.9.9

clean:
	rm -rf $(BUILD_DIR) $(TEST_BUILD_DIR)
