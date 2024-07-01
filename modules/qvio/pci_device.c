#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "pci_device.h"
#include "device.h"

#include "libxdma_api.h"
#include "libxdma.h"

#include <linux/aer.h>

#define DRV_MODULE_NAME "qvio-pci"

static const struct pci_device_id __pci_ids[] = {
	{ PCI_DEVICE(0x12AB, 0x0710), },
	{0,}
};
MODULE_DEVICE_TABLE(pci, __pci_ids);

static int __pci_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
	int err = 0;
	struct qvio_device* self;
	struct xdma_dev *xdev;
	void *hndl;

	pr_info("\n");

	self = qvio_device_new();
	if(! self) {
		pr_err("qvio_device_new() failed\n");
		err = -ENOMEM;
		goto err0;
	}

	self->pci_dev = pdev;
	dev_set_drvdata(&pdev->dev, self);

	self->user_max = MAX_USER_IRQ;
	self->h2c_channel_max = XDMA_CHANNEL_NUM_MAX;
	self->c2h_channel_max = XDMA_CHANNEL_NUM_MAX;

	hndl = xdma_device_open(DRV_MODULE_NAME, pdev, &self->user_max,
		&self->h2c_channel_max, &self->c2h_channel_max);
	if (!hndl) {
		pr_err("xdma_device_open() failed\n");
		err = -EINVAL;
		goto err1;
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

	return 0;

err1:
	qvio_device_put(self);
err0:
	return err;
}

static void __pci_remove(struct pci_dev *pdev) {
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	pr_info("\n");

	if (! self)
		return;

	xdma_device_close(self->pci_dev, self->xdev);
	qvio_device_put(self);
	dev_set_drvdata(&pdev->dev, NULL);
}

static pci_ers_result_t __pci_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
{
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	switch (state) {
	case pci_channel_io_normal:
		return PCI_ERS_RESULT_CAN_RECOVER;
	case pci_channel_io_frozen:
		pr_warn("dev 0x%p,0x%p, frozen state error, reset controller\n",
			pdev, self);
		xdma_device_offline(pdev, self->xdev);
		pci_disable_device(pdev);
		return PCI_ERS_RESULT_NEED_RESET;
	case pci_channel_io_perm_failure:
		pr_warn("dev 0x%p,0x%p, failure state error, req. disconnect\n",
			pdev, self);
		return PCI_ERS_RESULT_DISCONNECT;
	}
	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t __pci_slot_reset(struct pci_dev *pdev)
{
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	pr_info("0x%p restart after slot reset\n", self);
	if (pci_enable_device_mem(pdev)) {
		pr_info("0x%p failed to renable after slot reset\n", self);
		return PCI_ERS_RESULT_DISCONNECT;
	}

	pci_set_master(pdev);
	pci_restore_state(pdev);
	pci_save_state(pdev);
	xdma_device_online(pdev, self->xdev);

	return PCI_ERS_RESULT_RECOVERED;
}

static void __pci_error_resume(struct pci_dev *pdev)
{
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	pr_info("dev 0x%p,0x%p.\n", pdev, self);
#if PCI_AER_NAMECHANGE
	pci_aer_clear_nonfatal_status(pdev);
#else
	pci_cleanup_aer_uncorrect_error_status(pdev);
#endif

}

#if KERNEL_VERSION(4, 13, 0) <= LINUX_VERSION_CODE
static void __pci_reset_prepare(struct pci_dev *pdev)
{
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	pr_info("dev 0x%p,0x%p.\n", pdev, self);
	xdma_device_offline(pdev, self->xdev);
}

static void __pci_reset_done(struct pci_dev *pdev)
{
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	pr_info("dev 0x%p,0x%p.\n", pdev, self);
	xdma_device_online(pdev, self->xdev);
}

#elif KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
static void __pci_reset_notify(struct pci_dev *pdev, bool prepare)
{
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	pr_info("dev 0x%p,0x%p, prepare %d.\n", pdev, self, prepare);

	if (prepare)
		xdma_device_offline(pdev, self->xdev);
	else
		xdma_device_online(pdev, self->xdev);
}
#endif

static const struct pci_error_handlers __pci_err_handler = {
	.error_detected	= __pci_error_detected,
	.slot_reset	= __pci_slot_reset,
	.resume		= __pci_error_resume,
#if KERNEL_VERSION(4, 13, 0) <= LINUX_VERSION_CODE
	.reset_prepare	= __pci_reset_prepare,
	.reset_done	= __pci_reset_done,
#elif KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
	.reset_notify	= __pci_reset_notify,
#endif
};

static struct pci_driver pci_driver = {
	.name = DRV_MODULE_NAME,
	.id_table = __pci_ids,
	.probe = __pci_probe,
	.remove = __pci_remove,
	.err_handler = &__pci_err_handler,
};

int qvio_device_pci_register(void) {
	int err;

	pr_info("\n");

	err = pci_register_driver(&pci_driver);
	if(err) {
		pr_err("pci_register_driver() failed\n");
		goto err0;
	}

	return err;

err0:
	return err;
}

void qvio_device_pci_unregister(void) {
	pr_info("\n");

	pci_unregister_driver(&pci_driver);
}
