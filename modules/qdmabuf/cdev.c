#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s: " fmt, __func__

#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dma-buf.h>
#include <linux/uaccess.h>

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
	g_qdmabuf_class = class_create(THIS_MODULE, QDMABUF_NODE_NAME);
	if (IS_ERR(g_qdmabuf_class)) {
		pr_err("%s(#%d): class_create() failed, g_qdmabuf_class=%p\n", __func__, __LINE__, g_qdmabuf_class);

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
		pr_err("%s(#%d): alloc_chrdev_region() failed, err=%d\n", __func__, __LINE__, err);
		goto err0;
	}

	g_major = MAJOR(dev);

	pr_info("g_major=%d\n", g_major);

	g_qdmabuf_cdev = devm_kzalloc(device, sizeof(struct qdmabuf_cdev), GFP_KERNEL);
	if(! g_qdmabuf_cdev) {
		pr_err("%s(#%d): devm_kzalloc() failed\n", __func__, __LINE__);
		err = -ENOMEM;
		goto err1;
	}

	g_qdmabuf_cdev->device = get_device(device);
	g_qdmabuf_cdev->cdevno = MKDEV(g_major, 0);

	cdev_init(&g_qdmabuf_cdev->cdev, &qdmabuf_fops);
	g_qdmabuf_cdev->cdev.owner = THIS_MODULE;

	err = cdev_add(&g_qdmabuf_cdev->cdev, g_qdmabuf_cdev->cdevno, 1);
	if (err) {
		pr_err("%s(#%d): cdev_add() failed, err=%d\n", __func__, __LINE__, err);
		goto err2;
	}

	new_device = device_create(g_qdmabuf_class, NULL, g_qdmabuf_cdev->cdevno,
		g_qdmabuf_cdev, QDMABUF_NODE_NAME "%d", MINOR(g_qdmabuf_cdev->cdevno));
	if (IS_ERR(new_device)) {
		pr_err("%s(#%d): device_create() failed, new_device=%p\n", __func__, __LINE__, new_device);
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
	pr_info("%s(#%d): \n", __func__, __LINE__);

	filp->private_data = g_qdmabuf_cdev;
	nonseekable_open(inode, filp);

	return 0;
}

static long qdmabuf_ioctl_alloc(struct file * filp, unsigned long arg) {
	struct qdmabuf_alloc_args args;
	struct qdmabuf_cdev* qdmabuf_cdev = filp->private_data;
	long ret = 0;

	pr_info("%s(#%d): \n", __func__, __LINE__);

	ret = copy_from_user(&args, (void __user *)arg, sizeof(args));
	if (ret != 0) {
		pr_err("%s(#%d): copy_from_user() failed, err=%d\n", __func__, __LINE__, (int)ret);

		ret = -EFAULT;
		goto err;
	}

	switch(args.type) {
	case QDMABUF_ALLOC_TYPE_CONTIG:
		args.fd = qdmabuf_dmabuf_alloc_contig(qdmabuf_cdev->device, args.len, args.fd_flags);
		break;

	default:
		args.fd = -EINVAL;
		break;
	}

	if (args.fd < 0) {
		pr_err("%s(#%d): qdmabuf_dmabuf_alloc_xxx() failed, err=%d\n", __func__, __LINE__, args.fd);

		ret = args.fd;
		goto err;
	}

	ret = copy_to_user((void __user *)arg, &args, sizeof(args));
	if (ret != 0) {
		pr_err("%s(#%d): copy_to_user() failed, err=%d\n", __func__, __LINE__, (int)ret);

		ret = -EFAULT;
		goto err;
	}

err:
	return ret;
}

static long qdmabuf_ioctl_info(struct file * filp, unsigned long arg) {
	long ret;
	struct qdmabuf_info_args args;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct qdmabuf_cdev* qdmabuf_cdev = filp->private_data;
	int i;
	struct scatterlist *sg;

	pr_info("%s(#%d): \n", __func__, __LINE__);

	ret = copy_from_user(&args, (void __user *)arg, sizeof(args));
	if (ret != 0) {
		pr_err("%s(#%d): copy_from_user() failed, err=%d\n", __func__, __LINE__, (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	pr_info("fd=%d\n", args.fd);

	dmabuf = dma_buf_get(args.fd);
	if (IS_ERR(dmabuf)) {
		pr_err("%s(#%d): dma_buf_get() failed, dmabuf=%p\n", __func__, __LINE__, dmabuf);

		ret = -EINVAL;
		goto err0;
	}

	pr_info("dmabuf->size=%d\n", (int)dmabuf->size);

	attach = dma_buf_attach(dmabuf, qdmabuf_cdev->device);
	if (IS_ERR(attach)) {
		pr_err("%s(#%d): dma_buf_attach() failed, attach=%p\n", __func__, __LINE__, attach);

		ret = -EINVAL;
		goto err1;
	}

	pr_info("attach=%p\n", attach);

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		pr_err("%s(#%d): dma_buf_map_attachment() failed, sgt=%p\n", __func__, __LINE__, sgt);

		ret = -EINVAL;
		goto err2;
	}

	pr_info("sg_dma_len(sgt->sgl)=%d\n",
		(int)sg_dma_len(sgt->sgl));

	for_each_sgtable_dma_sg(sgt, sg, i) {
		pr_info("sg[%d]={.offset=%d, .length=%d .dma_address=%p}\n",
			i, sg->offset, sg->length, (void*)sg->dma_address);
	}

	ret = 0;

	dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
err2:
	dma_buf_detach(dmabuf, attach);
err1:
	dma_buf_put(dmabuf);
err0:
	return ret;
}

static long qdmabuf_file_ioctl(struct file * filp, unsigned int cmd, unsigned long arg) {
	long ret;

	pr_info("%s(#%d): \n", __func__, __LINE__);

	switch(cmd) {
	case QDMABUF_IOCTL_ALLOC:
		ret = qdmabuf_ioctl_alloc(filp, arg);
		break;

	case QDMABUF_IOCTL_INFO:
		ret = qdmabuf_ioctl_info(filp, arg);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
