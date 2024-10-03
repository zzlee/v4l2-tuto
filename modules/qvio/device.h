#ifndef __QVIO_DEVICE_H__
#define __QVIO_DEVICE_H__

#include "cdev.h"
#include "video.h"
#include "libxdma.h"

#include <linux/platform_device.h>
#include <linux/pci.h>
#include <linux/cdev.h>

#define QVIO_PCI_DRIVER_NAME QVIO_DRIVER_NAME "-pci"
#define QVIO_MAX_VIDEO 32

struct qvio_device {
	struct kref ref;

	struct device *dev;
	struct platform_device *pdev;
	struct pci_dev *pci_dev;
	uint32_t device_id;

	struct qvio_cdev cdev;

	// xdma
	int user_max;
	int c2h_channel_max;
	int h2c_channel_max;

#if 1 // USE_LIBXDMA
	struct xdma_dev *xdev;
#endif // USE_LIBXDMA

	struct qvio_video* video[QVIO_MAX_VIDEO];
};

struct qvio_device* qvio_device_new(void);
struct qvio_device* qvio_device_get(struct qvio_device* self);
void qvio_device_put(struct qvio_device* self);

int qvio_device_xdma_open(struct qvio_device* self, const char* mod_name);
void qvio_device_xdma_close(struct qvio_device* self);
void qvio_device_xdma_online(struct qvio_device* self, struct pci_dev *pdev);
void qvio_device_xdma_offline(struct qvio_device* self, struct pci_dev *pdev);

#endif // __QVIO_DEVICE_H__
