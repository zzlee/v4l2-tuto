KERNELRELEASE ?= $(shell uname -r)
KERNEL_DIR ?= /lib/modules/$(KERNELRELEASE)/build
PWD := $(shell pwd)

obj-m += qvio.o
qvio-objs := \
	module.o \
	queue.o \
	video.o \
	device.o \
	ioctl.o \
	user_job.o \
	platform_device.o

qvio-objs += \
	pci_device.o \
	libxdma.o \
	xdma_thread.o

ccflags-y += \
-DQVIO_MODULE_VERSION=\"$(MODULE_VERSION)\"

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

modules_install:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules_install
