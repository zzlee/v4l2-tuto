#ifndef __QVIO_DEVICE_H__
#define __QVIO_DEVICE_H__

#include "video.h"

#include <linux/platform_device.h>
#include <linux/pci.h>

#include "libxdma.h"
#include "xdma_thread.h"

#define QVIO_PCI_DRIVER_NAME QVIO_DRIVER_NAME "-pci"
#define QVIO_MAX_VIDEO 32

struct qvio_device {
	struct kref ref;

	struct device *dev;
	struct platform_device *pdev;
	struct pci_dev *pci_dev;

	// xdma
	struct xdma_dev *xdev;
	int user_max;
	int c2h_channel_max;
	int h2c_channel_max;

	struct qvio_video* video[QVIO_MAX_VIDEO];
};

struct qvio_device* qvio_device_new(void);
struct qvio_device* qvio_device_get(struct qvio_device* self);
void qvio_device_put(struct qvio_device* self);

#endif // __QVIO_DEVICE_H__
