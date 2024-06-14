#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include <linux/version.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dma-buf.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

MODULE_IMPORT_NS(DMA_BUF);

#include "cdev.h"
#include "uapi/qdmabuf.h"
#include "dmabuf_exp.h"

#define QDMABUF_NODE_NAME		"qdmabuf"
#define QDMABUF_MINOR_BASE		(0)
#define QDMABUF_MINOR_COUNT		(255)

static struct class *g_qdmabuf_class = NULL;
static int g_major = 0;
static struct qdmabuf_cdev* g_qdmabuf_cdev = NULL;

static int qdmabuf_file_open(struct inode *inode, struct file *filp);
static long qdmabuf_file_ioctl(struct file * filp, unsigned int cmd, unsigned long arg);

static struct file_operations qdmabuf_fops = {
	.open = qdmabuf_file_open,
	.unlocked_ioctl = qdmabuf_file_ioctl,
};

int qdmabuf_cdev_init(void) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6,4,0)
	g_qdmabuf_class = class_create(THIS_MODULE, QDMABUF_NODE_NAME);
#else
	g_qdmabuf_class = class_create(QDMABUF_NODE_NAME);
#endif
	if (IS_ERR(g_qdmabuf_class)) {
		pr_err("class_create() failed, g_qdmabuf_class=%p\n", g_qdmabuf_class);

		return -EINVAL;
	}

	return 0;
}

void qdmabuf_cdev_cleanup(void) {
	if(g_qdmabuf_cdev) {
		device_destroy(g_qdmabuf_class, g_qdmabuf_cdev->cdevno);
		cdev_del(&g_qdmabuf_cdev->cdev);
		put_device(g_qdmabuf_cdev->device);
		devm_kfree(g_qdmabuf_cdev->device, g_qdmabuf_cdev);
		g_qdmabuf_cdev = NULL;
	}

	if(g_major) {
		unregister_chrdev_region(MKDEV(g_major, QDMABUF_MINOR_BASE), QDMABUF_MINOR_COUNT);
		g_major = 0;
	}

	if(g_qdmabuf_class) {
		class_destroy(g_qdmabuf_class);
		g_qdmabuf_class = NULL;
	}
}

int qdmabuf_cdev_create_interfaces(struct device* device) {
	int err;
	dev_t dev;
	struct device* new_device;

	err = alloc_chrdev_region(&dev, QDMABUF_MINOR_BASE, QDMABUF_MINOR_COUNT, QDMABUF_NODE_NAME);
	if (err) {
		pr_err("alloc_chrdev_region() failed, err=%d\n", err);
		goto err0;
	}

	g_major = MAJOR(dev);

	pr_info("g_major=%d\n", g_major);

	g_qdmabuf_cdev = devm_kzalloc(device, sizeof(struct qdmabuf_cdev), GFP_KERNEL);
	if(! g_qdmabuf_cdev) {
		pr_err("devm_kzalloc() failed\n");
		err = -ENOMEM;
		goto err1;
	}

	g_qdmabuf_cdev->device = get_device(device);
	g_qdmabuf_cdev->cdevno = MKDEV(g_major, 0);

	cdev_init(&g_qdmabuf_cdev->cdev, &qdmabuf_fops);
	g_qdmabuf_cdev->cdev.owner = THIS_MODULE;

	g_qdmabuf_cdev->wq_event = 0;
	init_waitqueue_head(&g_qdmabuf_cdev->wq_head);

	err = cdev_add(&g_qdmabuf_cdev->cdev, g_qdmabuf_cdev->cdevno, 1);
	if (err) {
		pr_err("cdev_add() failed, err=%d\n", err);
		goto err2;
	}

	new_device = device_create(g_qdmabuf_class, NULL, g_qdmabuf_cdev->cdevno,
		g_qdmabuf_cdev, QDMABUF_NODE_NAME);
	if (IS_ERR(new_device)) {
		pr_err("device_create() failed, new_device=%p\n", new_device);
		goto err3;
	}

	return 0;

err3:
	cdev_del(&g_qdmabuf_cdev->cdev);
err2:
	put_device(g_qdmabuf_cdev->device);
	devm_kfree(device, g_qdmabuf_cdev);
	g_qdmabuf_cdev = NULL;
err1:
	unregister_chrdev_region(MKDEV(g_major, QDMABUF_MINOR_BASE), QDMABUF_MINOR_COUNT);
	g_major = 0;
err0:

	return err;
}

static int qdmabuf_file_open(struct inode *inode, struct file *filp) {
	struct qdmabuf_cdev* qdmabuf_cdev = container_of(inode->i_cdev, struct qdmabuf_cdev, cdev);

	pr_info("\n");

	filp->private_data = qdmabuf_cdev;
	nonseekable_open(inode, filp);

	return 0;
}

static long qdmabuf_ioctl_alloc(struct file * filp, unsigned long arg) {
	struct qdmabuf_alloc_args args;
	struct qdmabuf_cdev* qdmabuf_cdev = filp->private_data;
	long ret = 0;

	pr_info("\n");

	ret = copy_from_user(&args, (void __user *)arg, sizeof(args));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err;
	}

	switch(args.type) {
	case QDMABUF_TYPE_DMA_CONTIG:
		ret = qdmabuf_dmabuf_alloc_dma_contig(
			qdmabuf_cdev->device, args.len, args.fd_flags, args.dma_dir);
		break;

	case QDMABUF_TYPE_DMA_SG:
		ret = qdmabuf_dmabuf_alloc_dma_sg(
			qdmabuf_cdev->device, args.len, args.fd_flags, args.dma_dir);
		break;

	case QDMABUF_TYPE_VMALLOC:
		ret = qdmabuf_dmabuf_alloc_vmalloc(
			qdmabuf_cdev->device, args.len, args.fd_flags, args.dma_dir);
		break;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,9,0)
#else
	case QDMABUF_TYPE_SYS_HEAP:
		ret = qdmabuf_dmabuf_alloc_sys_heap(
			qdmabuf_cdev->device, args.len, args.fd_flags, args.dma_dir);
		break;
#endif

	default:
		ret = -EINVAL;
		break;
	}

	if (ret < 0) {
		pr_err("qdmabuf_dmabuf_alloc_xxx() failed, err=%d\n", (int)ret);
		goto err;
	}

	args.fd = (__u32)ret;

	ret = copy_to_user((void __user *)arg, &args, sizeof(args));
	if (ret != 0) {
		pr_err("copy_to_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err;
	}

err:
	return ret;
}

static void sgt_dump(struct sg_table *sgt)
{
	int i;
	struct scatterlist *sg = sgt->sgl;

	pr_info("sgt 0x%p, sgl 0x%p, nents %u/%u.\n", sgt, sgt->sgl, sgt->nents,
		sgt->orig_nents);

	for (i = 0; i < sgt->orig_nents; i++, sg = sg_next(sg)) {
		if(i > 8) {
			pr_info("... more pages ...");
			break;
		}

		pr_info("%d, 0x%p, pg 0x%p,%u+%u, dma 0x%llx,%u.\n", i, sg,
			sg_page(sg), sg->offset, sg->length, sg_dma_address(sg),
			sg_dma_len(sg));
	}
}

static long qdmabuf_ioctl_info(struct file * filp, unsigned long arg) {
	long ret;
	struct qdmabuf_info_args args;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct qdmabuf_cdev* qdmabuf_cdev = filp->private_data;

	pr_info("\n");

	ret = copy_from_user(&args, (void __user *)arg, sizeof(args));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	pr_info("fd=%d\n", args.fd);

	dmabuf = dma_buf_get(args.fd);
	if (IS_ERR(dmabuf)) {
		pr_err("dma_buf_get() failed, dmabuf=%p\n", dmabuf);

		ret = -EINVAL;
		goto err0;
	}

	pr_info("dmabuf->size=%d\n", (int)dmabuf->size);

	attach = dma_buf_attach(dmabuf, qdmabuf_cdev->device);
	if (IS_ERR(attach)) {
		pr_err("dma_buf_attach() failed, attach=%p\n", attach);

		ret = -EINVAL;
		goto err1;
	}

	pr_info("attach=%p\n", attach);

	ret = dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE);
	if (IS_ERR(attach)) {
		pr_err("dma_buf_begin_cpu_access() failed, attach=%p\n", attach);
		goto err2;
	}

	sgt = dma_buf_map_attachment(attach, DMA_FROM_DEVICE);
	if (IS_ERR(sgt)) {
		pr_err("dma_buf_map_attachment() failed, sgt=%p\n", sgt);

		ret = -EINVAL;
		goto err3;
	}

	sgt_dump(sgt);

	ret = 0;

	dma_buf_unmap_attachment(attach, sgt, DMA_FROM_DEVICE);
err3:
	dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
err2:
	dma_buf_detach(dmabuf, attach);
err1:
	dma_buf_put(dmabuf);
err0:
	return ret;
}

static long qdmabuf_ioctl_wq(struct file * filp, unsigned long arg) {
	long ret;
	struct qdmabuf_wq_args args;
	struct qdmabuf_cdev* qdmabuf_cdev = filp->private_data;
	__u32 last_wq_event;
	int err;

	pr_info("\n");

	ret = copy_from_user(&args, (void __user *)arg, sizeof(args));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	switch(args.type) {
	case 0: // consumer
		last_wq_event = qdmabuf_cdev->wq_event;
		err = wait_event_interruptible(qdmabuf_cdev->wq_head, (last_wq_event != qdmabuf_cdev->wq_event));
		if(err) {
			pr_err("wait_event_interruptible() failed, err=%d\n", err);
			ret = err;
			break;
		}

		pr_info("qdmabuf_cdev->wq_event=%d\n", (int)qdmabuf_cdev->wq_event);
		args.value = qdmabuf_cdev->wq_event;

		ret = copy_to_user((void __user *)arg, &args, sizeof(args));
		if (ret != 0) {
			pr_err("copy_to_user() failed, err=%d\n", (int)ret);

			ret = -EFAULT;
			goto err0;
		}

		ret = 0;
		break;

	case 1: // producer
		qdmabuf_cdev->wq_event = args.value;
		pr_info("qdmabuf_cdev->wq_event=%d\n", (int)qdmabuf_cdev->wq_event);

		wake_up_interruptible(&qdmabuf_cdev->wq_head);
		ret = 0;
		break;

	default:
		pr_err("unexpected value, args.type=%d\n", args.type);
		ret = -EINVAL;
		goto err0;
		break;
	}


err0:
	return ret;
}

static long qdmabuf_file_ioctl(struct file * filp, unsigned int cmd, unsigned long arg) {
	long ret;

	switch(cmd) {
	case QDMABUF_IOCTL_ALLOC:
		ret = qdmabuf_ioctl_alloc(filp, arg);
		break;

	case QDMABUF_IOCTL_INFO:
		ret = qdmabuf_ioctl_info(filp, arg);
		break;

	case QDMABUF_IOCTL_WQ:
		ret = qdmabuf_ioctl_wq(filp, arg);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
