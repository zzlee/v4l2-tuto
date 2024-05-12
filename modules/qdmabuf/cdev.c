#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s: " fmt, __func__

#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>

#include "qdmabuf.h"
#include "cdev.h"

#define QDMABUF_NODE_NAME		"qdmabuf"
#define QDMABUF_MINOR_BASE		(0)
#define QDMABUF_MINOR_COUNT		(255)

static struct class *g_qdmabuf_class = NULL;
static int g_major = 0;
static struct qdmabuf_cdev* g_qdmabuf_cdev = NULL;

static int qdmabuf_file_open(struct inode *inode, struct file *filp);
static int qdmabuf_file_release(struct inode *inode, struct file *filp);
static ssize_t qdmabuf_file_read(struct file *filp, char *buf, size_t size, loff_t *f_pos);
static ssize_t qdmabuf_file_write(struct file *filp, const char *buf, size_t size, loff_t *f_pos);
static long qdmabuf_file_ioctl(struct file * filp, unsigned int cmd, unsigned long arg);
static __poll_t qdmabuf_file_poll(struct file *filp, struct poll_table_struct *wait);
static int qdmabuf_mmap(struct file *filp, struct vm_area_struct *vma);

static struct file_operations qdmabuf_fops = {
	.open = qdmabuf_file_open,
	.release = qdmabuf_file_release,
	.read = qdmabuf_file_read,
	.write = qdmabuf_file_write,
	.unlocked_ioctl = qdmabuf_file_ioctl,
	.poll = qdmabuf_file_poll,
	.mmap = qdmabuf_mmap,
};

int qdmabuf_cdev_init(void) {
	g_qdmabuf_class = class_create(THIS_MODULE, QDMABUF_NODE_NAME);
	if (IS_ERR(g_qdmabuf_class)) {
		dbg_init(QDMABUF_NODE_NAME ": failed to create class");
		return -EINVAL;
	}

	return 0;
}

void qdmabuf_cdev_cleanup(void) {
	if(g_qdmabuf_cdev) {
		device_destroy(g_qdmabuf_class, g_qdmabuf_cdev->dev);
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
	g_qdmabuf_cdev->dev = MKDEV(g_major, 0);

	cdev_init(&g_qdmabuf_cdev->cdev, &qdmabuf_fops);
	g_qdmabuf_cdev->cdev.owner = THIS_MODULE;

	err = cdev_add(&g_qdmabuf_cdev->cdev, g_qdmabuf_cdev->dev, 1);
	if (err) {
		dev_err(device, "%s(#%d): cdev_add() fail, err=%d\n", __func__, __LINE__, err);
		goto cdev_add_failed;
	}

	g_qdmabuf_cdev->device = device_create(g_qdmabuf_class, NULL, g_qdmabuf_cdev->dev, NULL, QDMABUF_NODE_NAME "%d", MINOR(g_qdmabuf_cdev->dev));
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
	pr_err("%s(#%d): \n", __func__, __LINE__);

	return 0;
}

static int qdmabuf_file_release(struct inode *inode, struct file *filp) {
	pr_err("%s(#%d): \n", __func__, __LINE__);

	return 0;
}

static ssize_t qdmabuf_file_read(struct file *filp, char *buf, size_t size, loff_t *f_pos) {
	pr_err("%s(#%d): \n", __func__, __LINE__);

	return 0;
}

static ssize_t qdmabuf_file_write(struct file *filp, const char *buf, size_t size, loff_t *f_pos) {
	pr_err("%s(#%d): \n", __func__, __LINE__);

	return 0;
}

static long qdmabuf_file_ioctl(struct file * filp, unsigned int cmd, unsigned long arg) {
	pr_err("%s(#%d): \n", __func__, __LINE__);

	return 0;
}

static __poll_t qdmabuf_file_poll(struct file *filp, struct poll_table_struct *wait) {
	pr_err("%s(#%d): \n", __func__, __LINE__);

	return 0;
}

static int qdmabuf_mmap(struct file *filp, struct vm_area_struct *vma) {
	pr_err("%s(#%d): \n", __func__, __LINE__);

	return 0;
}
