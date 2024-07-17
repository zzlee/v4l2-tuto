#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "device.h"
#include "libxdma_api.h"

struct qvio_device* qvio_device_new(void) {
	int err;
	struct qvio_device* self = kzalloc(sizeof(struct qvio_device), GFP_KERNEL);

	if(! self) {
		pr_err("kzalloc() failed\n");
		err = -ENOMEM;
		goto err0;
	}

	kref_init(&self->ref);

	return self;

err0:
	return NULL;
}

struct qvio_device* qvio_device_get(struct qvio_device* self) {
	if (self)
		kref_get(&self->ref);

	return self;
}

static void __s_free(struct kref *ref)
{
	struct qvio_device* self = container_of(ref, struct qvio_device, ref);

	pr_info("\n");

	kfree(self);
}

void qvio_device_put(struct qvio_device* self) {
	if (self)
		kref_put(&self->ref, __s_free);
}

int qvio_device_xdma_open(struct qvio_device* self, const char* mod_name) {
	int err = 0;
	struct pci_dev* pdev = self->pci_dev;

#if 1 // USE_LIBXDMA
	struct xdma_dev *xdev;
	void *hndl;
#endif // USE_LIBXDMA

	self->user_max = MAX_USER_IRQ;
	self->h2c_channel_max = XDMA_CHANNEL_NUM_MAX;
	self->c2h_channel_max = XDMA_CHANNEL_NUM_MAX;

#if 1 // USE_LIBXDMA
	hndl = xdma_device_open(mod_name, pdev, &self->user_max,
		&self->h2c_channel_max, &self->c2h_channel_max);
	if (!hndl) {
		pr_err("xdma_device_open() failed\n");
		err = -EINVAL;
		goto err0;
	}

	if (self->user_max > MAX_USER_IRQ) {
		pr_err("Maximum users limit reached\n");
		err = -EINVAL;
		goto err1;
	}

	if (self->h2c_channel_max > XDMA_CHANNEL_NUM_MAX) {
		pr_err("Maximun H2C channel limit reached\n");
		err = -EINVAL;
		goto err1;
	}

	if (self->c2h_channel_max > XDMA_CHANNEL_NUM_MAX) {
		pr_err("Maximun C2H channel limit reached\n");
		err = -EINVAL;
		goto err1;
	}

	if (!self->h2c_channel_max && !self->c2h_channel_max)
		pr_warn("NO engine found!\n");

	if (self->user_max) {
		u32 mask = (1 << (self->user_max + 1)) - 1;

		err = xdma_user_isr_enable(hndl, mask);
		if (err) {
			pr_err("xdma_user_isr_enable() failed, err=%d\n", err);
			goto err1;
		}
	}

	/* make sure no duplicate */
	xdev = xdev_find_by_pdev(pdev);
	if (!xdev) {
		pr_warn("NO xdev found!\n");
		err = -EINVAL;
		goto err1;
	}

	if (hndl != xdev) {
		pr_err("xdev handle mismatch\n");
		err = -EINVAL;
		goto err1;
	}

	pr_info("%s xdma%d, pdev 0x%p, xdev 0x%p, usr %d, ch %d,%d.\n",
		dev_name(&pdev->dev), xdev->idx, pdev, xdev,
		self->user_max, self->h2c_channel_max,
		self->c2h_channel_max);

	self->xdev = hndl;
#endif // USE_LIBXDMA

	return err;

err1:
#if 1 // USE_LIBXDMA
	xdma_device_close(self->pci_dev, hndl);
#endif // USE_LIBXDMA
err0:
	return err;
}

void qvio_device_xdma_close(struct qvio_device* self) {
#if 1 // USE_LIBXDMA
	xdma_device_close(self->pci_dev, self->xdev);
#endif // USE_LIBXDMA
}

void qvio_device_xdma_online(struct qvio_device* self, struct pci_dev *pdev) {
#if 1 // USE_LIBXDMA
	xdma_device_online(pdev, self->xdev);
#endif // USE_LIBXDMA
}

void qvio_device_xdma_offline(struct qvio_device* self, struct pci_dev *pdev) {
#if 1 // USE_LIBXDMA
	xdma_device_offline(pdev, self->xdev);
#endif // USE_LIBXDMA
}
