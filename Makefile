CC ?= cc
CFLAGS ?= -Wall -Wextra -pedantic
CPPFLAGS ?=
LDFLAGS ?=
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/example
SOURCES := dlrl.c example.c
INCLUDES := -I. -Ithird_party/dotlottie_player/include -Ithird_party/raylib/include

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Normalize platform → prebuilt directory naming
DOTLOTTIE_OS := linux
ifeq ($(UNAME_S),Darwin)
DOTLOTTIE_OS := macos
endif
DOTLOTTIE_ARCH := $(UNAME_M)
ifeq ($(UNAME_M),aarch64)
DOTLOTTIE_ARCH := arm64
endif

DOTLOTTIE_LIB_DIR := third_party/dotlottie_player/lib/$(DOTLOTTIE_OS)/$(DOTLOTTIE_ARCH)
DOTLOTTIE_LIB := $(firstword $(wildcard $(DOTLOTTIE_LIB_DIR)/libdotlottie_player.*))
ifeq ($(strip $(DOTLOTTIE_LIB)),)
$(error Missing dotLottie prebuilts for $(DOTLOTTIE_OS)/$(DOTLOTTIE_ARCH). Grab binaries from dotlottie_player releases.)
endif
ifeq ($(shell [ -e "$(DOTLOTTIE_LIB)" ] && echo ok),)
$(error dotLottie library not found at $(DOTLOTTIE_LIB); ensure the binary exists.)
endif

RAYLIB_OS := $(DOTLOTTIE_OS)
RAYLIB_ARCH := $(DOTLOTTIE_ARCH)
RAYLIB_LIB_DIR := third_party/raylib/lib/$(RAYLIB_OS)/$(RAYLIB_ARCH)
RAYLIB_LIB := $(firstword $(wildcard $(RAYLIB_LIB_DIR)/libraylib.*))
ifeq ($(strip $(RAYLIB_LIB)),)
$(error Missing raylib prebuilts for $(RAYLIB_OS)/$(RAYLIB_ARCH). Place them in $(RAYLIB_LIB_DIR).)
endif
ifeq ($(shell [ -e "$(RAYLIB_LIB)" ] && echo ok),)
$(error raylib library not found at $(RAYLIB_LIB); ensure the binary exists.)
endif

LIB_DIRS := -L$(DOTLOTTIE_LIB_DIR) -L$(RAYLIB_LIB_DIR)
RPATH_FLAGS := -Wl,-rpath,$(abspath $(DOTLOTTIE_LIB_DIR)) -Wl,-rpath,$(abspath $(RAYLIB_LIB_DIR))

LDLIBS := $(LIB_DIRS) -ldotlottie_player -lraylib
LDFLAGS += $(RPATH_FLAGS)

ifeq ($(UNAME_S),Darwin)
LDLIBS += -framework Cocoa -framework IOKit -framework OpenGL
else
LDLIBS += -lGL -lm -lpthread -ldl -lrt -lX11
endif

.PHONY: all build clean run

all: build

build: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $^ -o $@ $(LDFLAGS) $(LDLIBS)

ASSET ?= super-man.lottie

run: build
	./$(TARGET) $(ASSET)

clean:
	rm -rf $(BUILD_DIR)
