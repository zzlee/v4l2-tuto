KERNELRELEASE ?= $(shell uname -r)
KERNEL_DIR ?= /lib/modules/$(KERNELRELEASE)/build
PWD := $(shell pwd)

obj-m += qdmabuf.o
qdmabuf-objs := \
	module.o \
	cdev.o

ccflags-y += \
-DQVCAM_MODULE_VERSION=\"$(MODULE_VERSION)\"

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
