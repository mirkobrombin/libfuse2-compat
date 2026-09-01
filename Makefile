CC ?= cc
AR ?= ar
INSTALL ?= install
PKG_CONFIG ?= pkg-config
FOUNDATIONC ?= foundationc
FOUNDATION_BACKEND ?= llvm

PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
DESTDIR ?=

CPPFLAGS ?=
CFLAGS ?= -O2 -g
LDFLAGS ?=

WARNINGS := -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
	-Wstrict-prototypes -Wmissing-prototypes -Werror=implicit-function-declaration
LIBS := -ldl -lpthread
LIB_HARDENING := -Wl,-z,defs -Wl,-z,relro,-z,now

BUILD_DIR := build
TEST_BUILD_DIR := tests/build
REAL_LIB := $(BUILD_DIR)/libfuse.so.2.9.9
SONAME_LINK := $(BUILD_DIR)/libfuse.so.2
DEV_LINK := $(BUILD_DIR)/libfuse.so
FOUNDATION_BUILD_DIR := $(BUILD_DIR)/foundation
FOUNDATION_ARCHIVE := $(FOUNDATION_BUILD_DIR)/lib/libfuse.a
FOUNDATION_INPUTS := foundation.package foundation.lock $(wildcard src/*.fn)

.PHONY: all clean check check-unit check-symbols check-system check-abi \
	check-legacy-loader check-appimage install uninstall

all: $(REAL_LIB) $(SONAME_LINK) $(DEV_LINK)

$(BUILD_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

$(FOUNDATION_ARCHIVE): $(FOUNDATION_INPUTS)
	$(FOUNDATIONC) build-library . -o $(FOUNDATION_BUILD_DIR) \
		--kind static --pic --backend $(FOUNDATION_BACKEND)

$(REAL_LIB): $(FOUNDATION_ARCHIVE) src/libfuse2-compat.map
	$(CC) -shared $(LDFLAGS) $(LIB_HARDENING) -Wl,-soname,libfuse.so.2 \
		-Wl,--version-script,src/libfuse2-compat.map \
		-o $@ -Wl,--whole-archive $(FOUNDATION_ARCHIVE) \
		-Wl,--no-whole-archive $(LIBS)

$(SONAME_LINK): $(REAL_LIB)
	ln -sfn $(notdir $(REAL_LIB)) $@

$(DEV_LINK): $(REAL_LIB)
	ln -sfn $(notdir $(REAL_LIB)) $@

$(TEST_BUILD_DIR)/libfuse3-test.so: tests/fake_fuse3.c src/fuse3_abi.h | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 -fPIC -D_GNU_SOURCE \
		$(filter-out -Wmissing-prototypes,$(WARNINGS)) \
		-Isrc $(LDFLAGS) -shared $< -o $@

$(TEST_BUILD_DIR)/test_bridge: tests/test_bridge.c src/fuse2_abi.h $(SONAME_LINK) | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(WARNINGS) -D_GNU_SOURCE -Isrc \
		$< -L$(BUILD_DIR) -Wl,-rpath,'$$ORIGIN/../../build' \
		$(LDFLAGS) -l:libfuse.so.2.9.9 -ldl -o $@

$(TEST_BUILD_DIR)/backend_probe: tests/backend_probe.c src/fuse2_abi.h $(SONAME_LINK) | $(TEST_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 $(WARNINGS) -D_GNU_SOURCE -Isrc \
		$< -L$(BUILD_DIR) -Wl,-rpath,'$$ORIGIN/../../build' \
		$(LDFLAGS) -l:libfuse.so.2.9.9 -o $@

check-unit: $(TEST_BUILD_DIR)/libfuse3-test.so $(TEST_BUILD_DIR)/test_bridge
	LIBFUSE2_COMPAT_BACKEND=$(abspath $(TEST_BUILD_DIR)/libfuse3-test.so) \
		$(TEST_BUILD_DIR)/test_bridge

check-symbols: all
	./tests/test_symbols.sh $(REAL_LIB)

check-system: $(TEST_BUILD_DIR)/backend_probe
	env -u LIBFUSE2_COMPAT_BACKEND $(TEST_BUILD_DIR)/backend_probe

check-abi:
	./tests/test_abi_layout.sh

check-legacy-loader: all
	./tests/fetch_legacy_fixture.sh
	./tests/test_legacy_loader.sh \
		tests/fixtures/obsolete-appimagetool-x86_64.AppImage

check-appimage: all
	@test -n "$(APPIMAGE)" || { \
		echo 'usage: make check-appimage APPIMAGE=/path/to/legacy.AppImage' >&2; \
		exit 2; \
	}
	./tests/integration_appimage.sh "$(APPIMAGE)"

check: check-unit check-symbols check-system check-abi

install: all
	$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0755 $(REAL_LIB) $(DESTDIR)$(LIBDIR)/libfuse.so.2.9.9
	ln -sfn libfuse.so.2.9.9 $(DESTDIR)$(LIBDIR)/libfuse.so.2

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/libfuse.so.2 $(DESTDIR)$(LIBDIR)/libfuse.so.2.9.9

clean:
	rm -rf $(BUILD_DIR) $(TEST_BUILD_DIR)
