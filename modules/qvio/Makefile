KERNELRELEASE ?= $(shell uname -r)
KERNEL_DIR ?= /lib/modules/$(KERNELRELEASE)/build
PWD := $(shell pwd)

obj-m += qvio.o
qvio-objs := \
	module.o \
	queue.o \
	device.o \
	cdev.o \
	video.o \
	ioctl.o \
	user_job.o \
	platform_device.o

qvio-objs += \
	pci_device.o \
	libxdma.o \
	xdma_thread.o

ccflags-y += \
-O3 \
-DQVIO_MODULE_VERSION=\"$(MODULE_VERSION)\"

# ccflags-y += -D__LIBXDMA_DEBUG__

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

modules_install:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules_install
