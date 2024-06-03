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

COMMON_DIR := ../common

# All common header files
CXXFLAGS += \
-std=c++11 \
-I${COMMON_DIR}

CFLAGS += \
-I${COMMON_DIR}

# All common dependent libraries
LDFLAGS += \
-pthread

ifeq (${BUILD_WITH_NVBUF},ON)

CXXFLAGS += \
-DBUILD_WITH_NVBUF=1 \
-I/usr/src/jetson_multimedia_api/include

LDFLAGS += \
-lnvbufsurface \
-L/usr/lib/aarch64-linux-gnu/tegra/ \
-Wl,-rpath-link=/usr/lib/aarch64-linux-gnu/tegra/

endif