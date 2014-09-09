# These should be defined in .geckopaths
#GECKO_ROOT = /Volumes/fennec/gecko-desktop
#GECKO_OBJ = $(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0
#For Linux:
#SDL_INCLUDE_PATH = path/to/sdl/header/files
#SDL_LIB_PATH = path/to/sdl/libs

DEPTH=../..

#include $(DEPTH)/.geckopaths
PLATFORM:=$(shell uname)
#include $(DEPTH)/platforms/common.mk
#include $(DEPTH)/platforms/$(PLATFORM).mk

BUILD_DIR=$(DEPTH)/build-$(PLATFORM)

all: $(BUILD_DIR)/libyajl.a

OBJECTS = \
$(BUILD_DIR)/yajl.o \
$(BUILD_DIR)/yajl_alloc.o \
$(BUILD_DIR)/yajl_buf.o \
$(BUILD_DIR)/yajl_encode.o \
$(BUILD_DIR)/yajl_gen.o \
$(BUILD_DIR)/yajl_lex.o \
$(BUILD_DIR)/yajl_parser.o \
$(BUILD_DIR)/yajl_tree.o \
$(BUILD_DIR)/yajl_version.o

$(BUILD_DIR)/libyajl.a: $(OBJECTS)
	$(AR) cr $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -I. $^ -c -o $@


