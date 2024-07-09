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

static int __user_ctrl_release(struct inode *inode, struct file *filp) {
	struct qvio_video* self = filp->private_data;

	pr_info("self=%p\n", self);

	return 0;
}

static ssize_t __user_ctrl_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	struct qvio_video* self = filp->private_data;
	struct xdma_dev *xdev;
	void __iomem *reg;
	u32 w;
	int rv;

	xdev = self->xdev;

	/* only 32-bit aligned and 32-bit multiples */
	if (*pos & 3)
		return -EPROTO;
	/* first address is BAR base plus file position offset */
	reg = xdev->bar[self->bar_idx] + *pos;
	//w = read_register(reg);
	w = ioread32(reg);
	dbg_sg("%s(@%p, count=%ld, pos=%d) value = 0x%08x\n",
			__func__, reg, (long)count, (int)*pos, w);
	rv = copy_to_user(buf, &w, 4);
	if (rv)
		dbg_sg("Copy to userspace failed but continuing\n");

	*pos += 4;
	return 4;
}

static ssize_t __user_ctrl_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	struct qvio_video* self = filp->private_data;
	struct xdma_dev *xdev;
	void __iomem *reg;
	u32 w;
	int rv;

	xdev = self->xdev;

	/* only 32-bit aligned and 32-bit multiples */
	if (*pos & 3)
		return -EPROTO;

	/* first address is BAR base plus file position offset */
	reg = xdev->bar[self->bar_idx] + *pos;
	rv = copy_from_user(&w, buf, 4);
	if (rv)
		pr_info("copy from user failed %d/4, but continuing.\n", rv);

	dbg_sg("%s(0x%08x @%p, count=%ld, pos=%d)\n",
			__func__, w, reg, (long)count, (int)*pos);
	//write_register(w, reg);
	iowrite32(w, reg);
	*pos += 4;
	return 4;
}

/* maps the PCIe BAR into user space for memory-like access using mmap() */
static int __user_ctrl_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct qvio_video* self = filp->private_data;
	struct xdma_dev *xdev;
	unsigned long off;
	unsigned long phys;
	unsigned long vsize;
	unsigned long psize;
	int rv;

	xdev = self->xdev;

	off = vma->vm_pgoff << PAGE_SHIFT;
	/* BAR physical address */
	phys = pci_resource_start(xdev->pdev, self->bar_idx) + off;
	vsize = vma->vm_end - vma->vm_start;
	/* complete resource */
	psize = pci_resource_end(xdev->pdev, self->bar_idx) -
		pci_resource_start(xdev->pdev, self->bar_idx) + 1 - off;

	pr_info("self->bar_idx = %d\n", self->bar_idx);
	pr_info("xdev = 0x%p\n", xdev);
	pr_info("pci_dev = 0x%08lx\n", (unsigned long)xdev->pdev);
	pr_info("off = 0x%lx, vsize 0x%lu, psize 0x%lu.\n", off, vsize, psize);
	pr_info("start = 0x%llx\n",
		(unsigned long long)pci_resource_start(xdev->pdev,
		xdev->user_bar_idx));
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
	vma->vm_flags |= VMEM_FLAGS;
	/* make MMIO accessible to user space */
	rv = io_remap_pfn_range(vma, vma->vm_start, phys >> PAGE_SHIFT,
			vsize, vma->vm_page_prot);
	dbg_sg("vma=0x%p, vma->vm_start=0x%lx, phys=0x%lx, size=%lu = %d\n",
		vma, vma->vm_start, phys >> PAGE_SHIFT, vsize, rv);

	if (rv)
		return -EAGAIN;
	return 0;
}

static long __user_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct qvio_video* self = filp->private_data;
	struct xdma_dev *xdev;

	xdev = self->xdev;
	if (!xdev) {
		pr_info("cmd %u, xdev NULL.\n", cmd);
		return -EINVAL;
	}
	pr_info("cmd 0x%x, xdev 0x%p, pdev 0x%p.\n", cmd, xdev, xdev->pdev);

	if (_IOC_TYPE(cmd) != QVID_IOC_MAGIC) {
		pr_err("cmd %u, bad magic 0x%x/0x%x.\n",
			 cmd, _IOC_TYPE(cmd), QVID_IOC_MAGIC);
		return -ENOTTY;
	}

	switch (cmd) {
	case QVID_IOC_IOCOFFLINE:
		xdma_device_offline(xdev->pdev, xdev);
		break;

	case QVID_IOC_IOCONLINE:
		xdma_device_online(xdev->pdev, xdev);
		break;

	default:
		pr_err("UNKNOWN ioctl cmd 0x%x.\n", cmd);
		return -ENOTTY;
	}
	return 0;
}

static const struct file_operations user_ctrl_fops = {
	.owner = THIS_MODULE,
	.release = __user_ctrl_release,
	.read = __user_ctrl_read,
	.write = __user_ctrl_write,
	.mmap = __user_ctrl_mmap,
	.unlocked_ioctl = __user_ctrl_ioctl,
};

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

	self->dev = &pdev->dev;
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

	self->video[0] = qvio_video_new();
	if(! self) {
		pr_err("qvio_video_new() failed\n");
		err = -ENOMEM;
		goto err2;
	}

	self->video[0]->user_job_ctrl.enable = false;

	self->video[0]->vfl_dir = VFL_DIR_RX;
	self->video[0]->buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	self->video[0]->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	err = snprintf(self->video[0]->bus_info, sizeof(self->video[0]->bus_info), "PCI:%s", pci_name(pdev));
	if(err >= sizeof(self->video[0]->bus_info)) {
		pr_err("out of space, err=%d\n", err);
		self->video[0]->bus_info[sizeof(self->video[0]->bus_info) - 1] = '\0';
	}
	snprintf(self->video[0]->v4l2_dev.name, V4L2_DEVICE_NAME_SIZE, "qvio-rx");
	self->video[0]->queue.dev = self->dev;
	self->video[0]->queue.xdev = self->xdev;
	self->video[0]->queue.channel = 0;

	self->video[0]->user_ctrl_fops = &user_ctrl_fops;

	self->video[0]->xdev = self->xdev;
	self->video[0]->bar_idx = self->xdev->user_bar_idx;

	err = qvio_video_start(self->video[0]);
	if(err) {
		pr_err("qvio_qvio_start() failed, err=%d\n", err);
		goto err3;
	}

	return 0;

err3:
	qvio_video_put(self->video[0]);
err2:
	xdma_device_close(self->pci_dev, self->xdev);
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
