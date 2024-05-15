#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s: " fmt, __func__

#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>

#include "cdev.h"
#include "uapi/qdmabuf.h"

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

static long qdmabuf_ioctl_alloc(struct qdmabuf_cdev *qdmabuf_cdev, unsigned long arg);

int qdmabuf_cdev_init(void) {
	g_qdmabuf_class = class_create(THIS_MODULE, QDMABUF_NODE_NAME);
	if (IS_ERR(g_qdmabuf_class)) {
		pr_err(QDMABUF_NODE_NAME ": failed to create class");
		return -EINVAL;
	}

	return 0;
}

void qdmabuf_cdev_cleanup(void) {
	if(g_qdmabuf_cdev) {
		device_destroy(g_qdmabuf_class, g_qdmabuf_cdev->cdevno);
		cdev_del(&g_qdmabuf_cdev->cdev);
		devm_kfree(g_qdmabuf_cdev->parent_device, g_qdmabuf_cdev);
		g_qdmabuf_cdev = NULL;
	}

	if(g_major) {
		unregister_chrdev_region(MKDEV(g_major, QDMABUF_MINOR_BASE), QDMABUF_MINOR_COUNT);
	}

	if(g_qdmabuf_class) {
		class_destroy(g_qdmabuf_class);
	}
}

int qdmabuf_cdev_create_interfaces(struct device* device) {
	int err;
	dev_t dev;

	err = alloc_chrdev_region(&dev, QDMABUF_MINOR_BASE, QDMABUF_MINOR_COUNT, QDMABUF_NODE_NAME);
	if (err) {
		dev_err(device, "%s(#%d): alloc_chrdev_region() fail, err=%d\n", __func__, __LINE__, err);
		goto alloc_chrdev_region_failed;
	}

	g_major = MAJOR(dev);

	dev_info(device, "g_major=%d\n", g_major);

	g_qdmabuf_cdev = devm_kzalloc(device, sizeof(struct qdmabuf_cdev), GFP_KERNEL);
	if(! g_qdmabuf_cdev) {
		dev_err(device, "%s(#%d): devm_kzalloc() fail\n", __func__, __LINE__);
		err = -ENOMEM;
		goto qdmabuf_cdev_alloc_failed;
	}

	g_qdmabuf_cdev->parent_device = device;
	g_qdmabuf_cdev->cdevno = MKDEV(g_major, 0);

	cdev_init(&g_qdmabuf_cdev->cdev, &qdmabuf_fops);
	g_qdmabuf_cdev->cdev.owner = THIS_MODULE;

	err = cdev_add(&g_qdmabuf_cdev->cdev, g_qdmabuf_cdev->cdevno, 1);
	if (err) {
		dev_err(device, "%s(#%d): cdev_add() fail, err=%d\n", __func__, __LINE__, err);
		goto cdev_add_failed;
	}

	g_qdmabuf_cdev->device = device_create(g_qdmabuf_class, NULL, g_qdmabuf_cdev->cdevno,
		g_qdmabuf_cdev, QDMABUF_NODE_NAME "%d", MINOR(g_qdmabuf_cdev->cdevno));
	if (IS_ERR(g_qdmabuf_cdev->device)) {
		dev_err(device, "%s(#%d): device_create() fail, err=%d\n", __func__, __LINE__, err);
		goto device_create_failed;
	}

	return 0;

device_create_failed:
	cdev_del(&g_qdmabuf_cdev->cdev);
cdev_add_failed:
	devm_kfree(device, g_qdmabuf_cdev);
	g_qdmabuf_cdev = NULL;
qdmabuf_cdev_alloc_failed:
	unregister_chrdev_region(MKDEV(g_major, QDMABUF_MINOR_BASE), QDMABUF_MINOR_COUNT);
alloc_chrdev_region_failed:

	return err;
}

static int qdmabuf_file_open(struct inode *inode, struct file *filp) {
	pr_info("%s(#%d): \n", __func__, __LINE__);

	filp->private_data = g_qdmabuf_cdev;
	nonseekable_open(inode, filp);

	return 0;
}

static long qdmabuf_file_ioctl(struct file * filp, unsigned int cmd, unsigned long arg) {
	struct qdmabuf_cdev *qdmabuf_cdev = filp->private_data;
	long ret;

	pr_info("%s(#%d): \n", __func__, __LINE__);

	switch(cmd) {
	case QDMABUF_IOCTL_ALLOC:
		ret = qdmabuf_ioctl_alloc(qdmabuf_cdev, arg);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static long qdmabuf_ioctl_alloc(struct qdmabuf_cdev *qdmabuf_cdev, unsigned long arg) {
	struct qdmabuf_allocation_data args;
	long ret;

	pr_info("%s(#%d): \n", __func__, __LINE__);

	if (copy_from_user(&args, (void __user *)arg, sizeof(args)) != 0) {
		ret = -EFAULT;
		goto err;
	}

	ret = 0;

err:
	return ret;
}