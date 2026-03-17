CC ?= cc
INSTALL ?= install

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
REAL_LIB := $(BUILD_DIR)/libfuse.so.2.9.9
SONAME_LINK := $(BUILD_DIR)/libfuse.so.2
DEV_LINK := $(BUILD_DIR)/libfuse.so
OBJECTS := $(BUILD_DIR)/bridge.o $(BUILD_DIR)/fuse3_loader.o

.PHONY: all clean

all: $(REAL_LIB) $(SONAME_LINK) $(DEV_LINK)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COMMON_CFLAGS) -Isrc -c $< -o $@

$(REAL_LIB): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libfuse.so.2 -o $@ $(OBJECTS) $(LIBS)

$(SONAME_LINK): $(REAL_LIB)
	ln -sfn $(notdir $(REAL_LIB)) $@

$(DEV_LINK): $(REAL_LIB)
	ln -sfn $(notdir $(REAL_LIB)) $@

clean:
	rm -rf $(BUILD_DIR)
