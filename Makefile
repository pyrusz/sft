CC ?= gcc
LD ?= gcc
CFLAGS := ${CFLAGS}
CFLAGS += -march=native -O2 -std=c17 -D_POSIX_C_SOURCE=200809L -Wall -Wpedantic

CFLAGS += $(shell pkg-config --cflags xcb xcb-util xcb-renderutil fontconfig freetype2)
LDLIBS := $(shell pkg-config --libs xcb xcb-util xcb-renderutil fontconfig freetype2)

test: test.o fonts.o

.PHONY: clean
clean:
	-rm test test.o fonts.o
