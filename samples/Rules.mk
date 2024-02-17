# Clear the flags from env
CPPFLAGS :=
LDFLAGS :=

# Verbose flag
ifeq ($(VERBOSE), 1)
AT =
else
AT = @
endif

# Location of the target rootfs
ifeq ($(shell uname -m), aarch64)
TARGET_ROOTFS :=
else
ifeq ($(TARGET_ROOTFS),)
$(error Please specify the target rootfs path if you are cross-compiling)
endif
endif

ifeq ($(shell uname -m), aarch64)
CROSS_COMPILE :=
else
CROSS_COMPILE ?= aarch64-unknown-linux-gnu-
endif
AS             = $(AT) $(CROSS_COMPILE)as
LD             = $(AT) $(CROSS_COMPILE)ld
CC             = $(AT) $(CROSS_COMPILE)gcc
CPP            = $(AT) $(CROSS_COMPILE)g++
AR             = $(AT) $(CROSS_COMPILE)ar
NM             = $(AT) $(CROSS_COMPILE)nm
STRIP          = $(AT) $(CROSS_COMPILE)strip
OBJCOPY        = $(AT) $(CROSS_COMPILE)objcopy
OBJDUMP        = $(AT) $(CROSS_COMPILE)objdump

# Specify the logical root directory for headers and libraries.
ifneq ($(TARGET_ROOTFS),)
CPPFLAGS += --sysroot=$(TARGET_ROOTFS)
LDFLAGS +=
endif

# All common header files
CPPFLAGS += -std=c++11

# All common dependent libraries
LDFLAGS +=
