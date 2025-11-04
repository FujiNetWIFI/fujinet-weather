SHELL := /usr/bin/env bash
TARGET_EXEC ?= weather.bin
TARGET_DISK ?= weather.dsk
BUILD_DIR ?= ./build
SRC_DIRS ?= ./src
# Set this to the version of FN-LIB you wish to use in this project:
export CURRENT_TARGET := coco
export FUJINET_LIB_VERSION := 4.8.1

FLAGS=
LDFLAGS=-o $(TARGET_EXEC)

-include ../makefiles/fujinet-lib.mk

CC=cmoc
ASM=lwasm
CP=cp
WRITECOCOFILE=writecocofile
DECB=decb

SRCS := $(shell find $(SRC_DIRS) -name *.cpp -or -name *.c -or -name *.s)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

LIBFLAGS := $(foreach lib, $(LIBS), -L $(dir $(lib)))
LIBFLAGS += $(foreach lib,$(LIBS),-l$(notdir $(lib)))

CFLAGS += -Wno-const

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBFLAGS) 
	$(RM) $(TARGET_DISK)
	$(DECB) dskini $(TARGET_DISK)
	$(WRITECOCOFILE) $(TARGET_DISK) $(TARGET_EXEC)
#	$(CP) weather.dsk ~/Workspace/tnfs

# c source
$(BUILD_DIR)/%.c.o: %.c $(ALL_TASKS)
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $< 

# assy source
$(BUILD_DIR)/%.s.o: %.s $(ALL_TASKS)
	$(MKDIR_P) $(dir $@)
	$(ASM) -fobj --output=$@ $<

.PHONY: clean $(ALL_TASKS)

clean:
	$(RM) -r weather* $(BUILD_DIR)

-include $(DEPS)

MKDIR_P ?= mkdir -p
