KERNEL_ROOT ?= $(shell realpath /lib/modules/$(shell uname -r))
KERNEL_DIR ?= $(KERNEL_ROOT)/build
COPY = cp -f
MKDIR = mkdir -p
RMDIR = rm -rvf
PWD := $(shell pwd)
DKMS := dkms
DESTDIR :=
PREFIX := $(DESTDIR)/usr/src
INSTALLDIR := $(PREFIX)/$(MODULE_NAME)-$(MODULE_VERSION)

MODULE_NAME = qvcam
MODULE_VERSION = 0.0.1

obj-m += $(MODULE_NAME).o
$(MODULE_NAME)-objs := \
	module.o \
	attributes.o \
	buffers.o \
	controls.o \
	device.o \
	driver.o \
	file_read.o \
	format.o \
	frame.o \
	frame_filter.o \
	ioctl.o \
	list.o \
	log.o \
	map.o \
	rbuffer.o \
	settings.o \
	utils.o

ccflags-y += \
-DQVCAM_MODULE_VERSION=\"$(MODULE_VERSION)\"

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
