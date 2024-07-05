#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "cdev.h"
#include "ioctl.h"
#include "uapi/qdmabuf.h"

#include <linux/version.h>
#include <linux/device.h>
#include <linux/fs.h>

#define QDMABUF_NODE_NAME		"qdmabuf"
#define QDMABUF_MINOR_BASE		(0)
#define QDMABUF_MINOR_COUNT		(255)

static struct class *g_class = NULL;
static int g_major = 0;
static int g_cdevno_base = 0;

int qdmabuf_cdev_register(void) {
	int err;
	dev_t dev;

	pr_info("\n");

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6,4,0)
	g_class = class_create(THIS_MODULE, QDMABUF_NODE_NAME);
#else
	g_class = class_create(QDMABUF_NODE_NAME);
#endif
	if (IS_ERR(g_class)) {
		pr_err("class_create() failed, g_class=%p\n", g_class);
		err = -EINVAL;
		goto err0;
	}

	err = alloc_chrdev_region(&dev, QDMABUF_MINOR_BASE, QDMABUF_MINOR_COUNT, QDMABUF_NODE_NAME);
	if (err) {
		pr_err("alloc_chrdev_region() failed, err=%d\n", err);
		goto err1;
	}

	g_major = MAJOR(dev);
	pr_info("g_major=%d\n", g_major);

	g_cdevno_base = 0;

	return err;

err1:
	class_destroy(g_class);
err0:
	return err;
}

void qdmabuf_cdev_unregister(void) {
	pr_info("\n");

	unregister_chrdev_region(MKDEV(g_major, QDMABUF_MINOR_BASE), QDMABUF_MINOR_COUNT);
	class_destroy(g_class);
}

void qdmabuf_cdev_init(struct qdmabuf_device* self) {
	self->cdevno = 0;
	memset(&self->cdev, 0, sizeof(struct cdev));
}

static int __file_open(struct inode *inode, struct file *filp) {
	struct qdmabuf_device* device = container_of(inode->i_cdev, struct qdmabuf_device, cdev);

	pr_info("device=%p\n", device);

	filp->private_data = device;
	nonseekable_open(inode, filp);

	return 0;
}

static long __file_ioctl(struct file * filp, unsigned int cmd, unsigned long arg) {
	long ret;
	struct qdmabuf_device* device = filp->private_data;

	switch(cmd) {
	case QDMABUF_IOCTL_ALLOC:
		ret = qdmabuf_ioctl_alloc(device, arg);
		break;

	case QDMABUF_IOCTL_INFO:
		ret = qdmabuf_ioctl_info(device, arg);
		break;

	case QDMABUF_IOCTL_WQ:
		ret = qdmabuf_ioctl_wq(device, arg);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct file_operations qdmabuf_fops = {
	.open = __file_open,
	.unlocked_ioctl = __file_ioctl,
};

int qdmabuf_cdev_start(struct qdmabuf_device* self) {
	int err;
	struct device* new_device;

	pr_info("\n");

	self->cdevno = MKDEV(g_major, g_cdevno_base++);
	cdev_init(&self->cdev, &qdmabuf_fops);
	self->cdev.owner = THIS_MODULE;
	err = cdev_add(&self->cdev, self->cdevno, 1);
	if (err) {
		pr_err("cdev_add() failed, err=%d\n", err);
		goto err0;
	}

	new_device = device_create(g_class, NULL, self->cdevno, self,
		QDMABUF_NODE_NAME "%d", MINOR(self->cdevno));
	if (IS_ERR(new_device)) {
		pr_err("device_create() failed, new_device=%p\n", new_device);
		goto err1;
	}

	return 0;

err1:
	cdev_del(&self->cdev);
err0:
	return err;
}

void qdmabuf_cdev_stop(struct qdmabuf_device* self) {
	pr_info("\n");

	device_destroy(g_class, self->cdevno);
	cdev_del(&self->cdev);
}
