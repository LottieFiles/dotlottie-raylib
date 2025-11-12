CC ?= cc
CFLAGS ?= -Wall -Wextra -pedantic
CPPFLAGS ?=
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/example
SOURCES := dlrl.c example.c
INCLUDES := -I. -Ilibs/dotlottie_player/include
BREW_PREFIX ?= $(shell brew --prefix 2>/dev/null)
ifneq ($(BREW_PREFIX),)
INCLUDES += -I$(BREW_PREFIX)/include
endif
LIB_DIRS := -Llibs/dotlottie_player/lib

LDLIBS := $(LIB_DIRS) -ldotlottie_player -lraylib
UNAME_S := $(shell uname -s)

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
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $^ -o $@ $(LDLIBS)

ASSET ?= super-man.lottie

run: build
	./$(TARGET) $(ASSET)

clean:
	rm -rf $(BUILD_DIR)
