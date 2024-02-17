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
	log.o \
	utils.o \
	module.o \
	driver.o

ccflags-y += \
-DQVCAM_MODULE_VERSION=\"$(MODULE_VERSION)\"

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
