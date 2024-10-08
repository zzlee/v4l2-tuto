#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "pci_device.h"
#include "device.h"

#include <linux/aer.h>

#define DRV_MODULE_NAME "qvio-pci"

static const struct pci_device_id __pci_ids[] = {
	{ PCI_DEVICE(0x12AB, 0x0710), },
	{ PCI_DEVICE(0x12AB, 0x0750), },
	{0,}
};
MODULE_DEVICE_TABLE(pci, __pci_ids);

static ssize_t __file_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	struct qvio_device* self = filp->private_data;

#if 1 // USE_LIBXDMA
	struct xdma_dev *xdev;
	void __iomem *reg;
	u32 w;
#endif // USE_LIBXDMA

	int rv;

#if 1 // USE_LIBXDMA
	xdev = self->xdev;
#endif // USE_LIBXDMA

	/* only 32-bit aligned and 32-bit multiples */
	if (*pos & 3)
		return -EPROTO;

#if 1 // USE_LIBXDMA
	/* first address is BAR base plus file position offset */
	reg = xdev->bar[xdev->user_bar_idx] + *pos;
	//w = read_register(reg);
	w = ioread32(reg);
	dbg_sg("%s(@%p, count=%ld, pos=%d) value = 0x%08x\n",
			__func__, reg, (long)count, (int)*pos, w);
	rv = copy_to_user(buf, &w, 4);
	if (rv)
		dbg_sg("Copy to userspace failed but continuing\n");
#endif // USE_LIBXDMA

	*pos += 4;
	return 4;
}

static ssize_t __file_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	struct qvio_device* self = filp->private_data;

#if 1 // USE_LIBXDMA
	struct xdma_dev *xdev;
	void __iomem *reg;
	u32 w;
#endif // USE_LIBXDMA

	int rv;

#if 1 // USE_LIBXDMA
	xdev = self->xdev;
#endif // USE_LIBXDMA

	/* only 32-bit aligned and 32-bit multiples */
	if (*pos & 3)
		return -EPROTO;

#if 1 // USE_LIBXDMA
	/* first address is BAR base plus file position offset */
	reg = xdev->bar[xdev->user_bar_idx] + *pos;
	rv = copy_from_user(&w, buf, 4);
	if (rv)
		pr_info("copy from user failed %d/4, but continuing.\n", rv);

	dbg_sg("%s(0x%08x @%p, count=%ld, pos=%d)\n",
			__func__, w, reg, (long)count, (int)*pos);
	//write_register(w, reg);
	iowrite32(w, reg);
#endif // USE_LIBXDMA

	*pos += 4;
	return 4;
}

/* maps the PCIe BAR into user space for memory-like access using mmap() */
static int __file_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct qvio_device* self = filp->private_data;

#if 1 // USE_LIBXDMA
	struct xdma_dev *xdev;
#endif // USE_LIBXDMA

	unsigned long off;
	unsigned long phys;
	unsigned long vsize;
	unsigned long psize;
	int rv;

#if 1 // USE_LIBXDMA
	xdev = self->xdev;
#endif // USE_LIBXDMA

	off = vma->vm_pgoff << PAGE_SHIFT;

#if 1 // USE_LIBXDMA
	/* BAR physical address */
	phys = pci_resource_start(xdev->pdev, xdev->user_bar_idx) + off;
	vsize = vma->vm_end - vma->vm_start;
	/* complete resource */
	psize = pci_resource_end(xdev->pdev, xdev->user_bar_idx) -
		pci_resource_start(xdev->pdev, xdev->user_bar_idx) + 1 - off;

	pr_info("xdev->user_bar_idx = %d\n", xdev->user_bar_idx);
	pr_info("xdev = 0x%p\n", xdev);
	pr_info("pci_dev = 0x%08lx\n", (unsigned long)xdev->pdev);
	pr_info("off = 0x%lx, vsize 0x%lu, psize 0x%lu.\n", off, vsize, psize);
	pr_info("start = 0x%llx\n",
		(unsigned long long)pci_resource_start(xdev->pdev,
		xdev->user_bar_idx));
#endif // USE_LIBXDMA

	pr_info("phys = 0x%lx\n", phys);

	if (vsize > psize)
		return -EINVAL;
	/*
	 * pages must not be cached as this would result in cache line sized
	 * accesses to the end point
	 */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	/*
	 * prevent touching the pages (byte access) for swap-in,
	 * and prevent the pages from being swapped out
	 */
#if KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE
	vma->__vm_flags |= VMEM_FLAGS;
#else
	vma->vm_flags |= VMEM_FLAGS;
#endif

	/* make MMIO accessible to user space */
	rv = io_remap_pfn_range(vma, vma->vm_start, phys >> PAGE_SHIFT,
			vsize, vma->vm_page_prot);
	dbg_sg("vma=0x%p, vma->vm_start=0x%lx, phys=0x%lx, size=%lu = %d\n",
		vma, vma->vm_start, phys >> PAGE_SHIFT, vsize, rv);

	if (rv)
		return -EAGAIN;
	return 0;
}

static long __file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct qvio_device* self = filp->private_data;

#if 1 // USE_LIBXDMA
	struct xdma_dev *xdev;

	xdev = self->xdev;
	if (!xdev) {
		pr_info("cmd %u, xdev NULL.\n", cmd);
		return -EINVAL;
	}
	pr_info("cmd 0x%x, xdev 0x%p, pdev 0x%p.\n", cmd, xdev, xdev->pdev);
#endif // USE_LIBXDMA

	if (_IOC_TYPE(cmd) != QVID_IOC_MAGIC) {
		pr_err("cmd %u, bad magic 0x%x/0x%x.\n",
			 cmd, _IOC_TYPE(cmd), QVID_IOC_MAGIC);
		return -ENOTTY;
	}

#if 1 // USE_LIBXDMA
	switch (cmd) {
	case QVID_IOC_IOCOFFLINE:
		qvio_device_xdma_offline(self, xdev->pdev);
		break;

	case QVID_IOC_IOCONLINE:
		qvio_device_xdma_online(self, xdev->pdev);
		break;

	default:
		pr_err("UNKNOWN ioctl cmd 0x%x.\n", cmd);
		return -ENOTTY;
	}
#endif // USE_LIBXDMA

	return 0;
}

static const struct file_operations __fops = {
	.owner = THIS_MODULE,
	.open = qvio_cdev_open,
	.release = qvio_cdev_release,
	.read = __file_read,
	.write = __file_write,
	.mmap = __file_mmap,
	.unlocked_ioctl = __file_ioctl,
};

static int __pci_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
	int err = 0;
	struct qvio_device* self;

	pr_info("%04X:%04X (%04X:%04X)\n", (int)pdev->vendor, (int)pdev->device,
		(int)pdev->subsystem_vendor, (int)pdev->subsystem_device);

	self = qvio_device_new();
	if(! self) {
		pr_err("qvio_device_new() failed\n");
		err = -ENOMEM;
		goto err0;
	}

	self->dev = &pdev->dev;
	self->pci_dev = pdev;
	dev_set_drvdata(&pdev->dev, self);
	self->device_id = (((int)pdev->subsystem_vendor & 0xFFFF) << 16) |
		((int)pdev->subsystem_device & 0xFFFF);

	pr_info("self->device_id=%08X\n", self->device_id);

	self->user_max = MAX_USER_IRQ;
	self->h2c_channel_max = XDMA_CHANNEL_NUM_MAX;
	self->c2h_channel_max = XDMA_CHANNEL_NUM_MAX;

	err = qvio_device_xdma_open(self, DRV_MODULE_NAME);
	if (err) {
		pr_err("qvio_device_xdma_open() failed, err=%d\n", err);
		goto err1;
	}

	self->cdev.fops = &__fops;
	self->cdev.private_data = self;
	err = qvio_cdev_start(&self->cdev);
	if(err) {
		pr_err("qvio_cdev_start() failed, err=%d\n", err);
		goto err2;
	}

	self->video[0] = qvio_video_new();
	if(! self) {
		pr_err("qvio_video_new() failed\n");
		err = -ENOMEM;
		goto err3;
	}

	self->video[0]->qdev = self;
	self->video[0]->user_job_ctrl.enable = false;

	self->video[0]->vfl_dir = VFL_DIR_RX;
	self->video[0]->buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	self->video[0]->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	err = snprintf(self->video[0]->bus_info, sizeof(self->video[0]->bus_info), "PCI:%s", pci_name(pdev));
	if(err >= sizeof(self->video[0]->bus_info)) {
		pr_err("out of space, err=%d\n", err);
		self->video[0]->bus_info[sizeof(self->video[0]->bus_info) - 1] = '\0';
	}
	snprintf(self->video[0]->v4l2_dev.name, sizeof(self->video[0]->v4l2_dev.name), "qvio-rx");

	switch(self->device_id) {
	case 0xF7150002:
	case 0xF7570001:
		self->video[0]->channel = 0;
		break;

	case 0xF7570601:
		self->video[0]->channel = 0;
		break;

	default:
		pr_err("unexpected value, self->device_id=0x%08X\n", self->device_id);
		goto err4;
		break;
	}

	pr_info("self->video[0]->channel=%d\n", self->video[0]->channel);

	err = qvio_video_start(self->video[0]);
	if(err) {
		pr_err("qvio_qvio_start() failed, err=%d\n", err);
		goto err4;
	}

	return 0;

err4:
	qvio_video_put(self->video[0]);
err3:
	qvio_cdev_stop(&self->cdev);
err2:
	qvio_device_xdma_close(self);
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

	qvio_video_stop(self->video[0]);
	qvio_video_put(self->video[0]);
	qvio_cdev_stop(&self->cdev);
	qvio_device_xdma_close(self);
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
		qvio_device_xdma_offline(self, pdev);
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
	qvio_device_xdma_online(self, pdev);

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
	qvio_device_xdma_offline(self, pdev);
}

static void __pci_reset_done(struct pci_dev *pdev)
{
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	pr_info("dev 0x%p,0x%p.\n", pdev, self);
	qvio_device_xdma_online(self, pdev);
}

#elif KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
static void __pci_reset_notify(struct pci_dev *pdev, bool prepare)
{
	struct qvio_device* self = dev_get_drvdata(&pdev->dev);

	pr_info("dev 0x%p,0x%p, prepare %d.\n", pdev, self, prepare);

	if (prepare)
		qvio_device_xdma_offline(self, pdev);
	else
		qvio_device_xdma_online(self, pdev);
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
