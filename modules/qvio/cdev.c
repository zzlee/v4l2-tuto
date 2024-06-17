#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "device.h"
#include "ioctl.h"
#include "user_job.h"

#include <linux/version.h>
#include <linux/device.h>

#define QVIO_NODE_NAME		"qvio"
#define QVIO_MINOR_BASE		(0)
#define QVIO_MINOR_COUNT	(255)

static struct class *g_qvio_class = NULL;
static int g_major = 0;
static int g_cdevno_base = 0;

int qvio_cdev_register(void) {
	int err;
	dev_t dev;

	pr_info("\n");

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6,4,0)
	g_qvio_class = class_create(THIS_MODULE, QVIO_NODE_NAME);
#else
	g_qvio_class = class_create(QVIO_NODE_NAME);
#endif
	if (IS_ERR(g_qvio_class)) {
		pr_err("class_create() failed, g_qvio_class=%p\n", g_qvio_class);
		err = -EINVAL;
		goto err0;
	}

	err = alloc_chrdev_region(&dev, QVIO_MINOR_BASE, QVIO_MINOR_COUNT, QVIO_NODE_NAME);
	if (err) {
		pr_err("alloc_chrdev_region() failed, err=%d\n", err);
		goto err1;
	}

	g_major = MAJOR(dev);
	pr_info("g_major=%d\n", g_major);

	g_cdevno_base = 0;

	return err;

err1:
	class_destroy(g_qvio_class);
err0:
	return err;
}

void qvio_cdev_unregister(void) {
	pr_info("\n");

	unregister_chrdev_region(MKDEV(g_major, QVIO_MINOR_BASE), QVIO_MINOR_COUNT);
	class_destroy(g_qvio_class);
}

void qvio_cdev_init(struct qvio_device* self) {
	self->cdevno = 0;
	memset(&self->cdev, 0, sizeof(struct cdev));
}

static int qvio_file_open(struct inode *inode, struct file *filp) {
	struct qvio_device* device = container_of(inode->i_cdev, struct qvio_device, cdev);

	pr_info("device=%p\n", device);

	filp->private_data = device;
	nonseekable_open(inode, filp);

	return 0;
}

static long qvio_file_ioctl(struct file * filp, unsigned int cmd, unsigned long arg) {
	long ret;
	struct qvio_device* device = filp->private_data;

	switch (cmd) {
	case QVID_IOC_BUF_DONE:
		ret = qvio_device_buf_done(device);
		break;

	case QVID_IOC_USER_JOB: {
		struct qvio_user_job_args args;
		ret = qvio_user_job_wait(device, &args.user_job);
		if (ret != 0) {
			pr_err("qvio_user_job_wait() failed, err=%d\n", (int)ret);

			ret = -EFAULT;
			break;
		}

		ret = copy_to_user((void __user *)arg, &args, sizeof(args));
		if (ret != 0) {
			pr_err("copy_from_user() failed, err=%d\n", (int)ret);

			ret = -EFAULT;
			break;
		}
	}
		break;

	case QVID_IOC_USER_JOB_DONE: {
		struct qvio_user_job_done_args args;
		ret = copy_from_user(&args, (void __user *)arg, sizeof(args));
		if (ret != 0) {
			pr_err("copy_from_user() failed, err=%d\n", (int)ret);

			ret = -EFAULT;
			break;
		}

		ret = qvio_user_job_done(device, &args);
		if (ret != 0) {
			pr_err("qvio_user_job_done() failed, err=%d\n", (int)ret);

			ret = -EFAULT;
			break;
		}

		ret = copy_to_user((void __user *)arg, &args, sizeof(args));
		if (ret != 0) {
			pr_err("copy_from_user() failed, err=%d\n", (int)ret);

			ret = -EFAULT;
			break;
		}
	}
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct file_operations qvio_fops = {
	.open = qvio_file_open,
	.unlocked_ioctl = qvio_file_ioctl,
};

int qvio_cdev_start(struct qvio_device* self) {
	int err;
	struct device* new_device;

	pr_info("\n");

	self->cdevno = MKDEV(g_major, g_cdevno_base++);
	cdev_init(&self->cdev, &qvio_fops);
	self->cdev.owner = THIS_MODULE;
	err = cdev_add(&self->cdev, self->cdevno, 1);
	if (err) {
		pr_err("cdev_add() failed, err=%d\n", err);
		goto err0;
	}

	new_device = device_create(g_qvio_class, NULL, self->cdevno, self,
		QVIO_NODE_NAME "%d", MINOR(self->cdevno));
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

void qvio_cdev_stop(struct qvio_device* self) {
	pr_info("\n");

	device_destroy(g_qvio_class, self->cdevno);
	cdev_del(&self->cdev);
}
