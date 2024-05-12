KERNELRELEASE ?= $(shell uname -r)
KERNEL_DIR ?= /lib/modules/$(KERNELRELEASE)/build
PWD := $(shell pwd)

obj-m += qdmabuf.o
qdmabuf-objs := \
	qdmabuf.o \
	module.o \
	cdev.o

ccflags-y += \
-DQDMABUF_MODULE_VERSION=\"0.0.1\"

ifeq ($(DEBUG),1)
	ccflags-y += -D__QDMABUF_DEBUG__
endif

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
