# Verbose flag
ifeq ($(V), 1)
AT =
else
AT = @
endif

AS             ?= as
LD             ?= ld
CC             ?= gcc
CXX            ?= g++
AR             ?= ar
NM             ?= nm
STRIP          ?= strip
OBJCOPY        ?= objcopy
OBJDUMP        ?= objdump

# All common header files
CXXFLAGS += -std=c++11 \
-I../common
CFLAGS += \
-I../common

# All common dependent libraries
LDFLAGS += \
-pthread

COMMON_DIR := ../common

