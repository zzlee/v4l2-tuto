#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "version.h"
#include "device.h"
#include "cdev.h"

#define DRV_MODULE_DESC		"QCAP dma-buf Driver"

static char version[] = DRV_MODULE_DESC " v" DRV_MODULE_VERSION "\n";

MODULE_AUTHOR("ZzLab");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_LICENSE("Dual BSD/GPL");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
MODULE_IMPORT_NS(DMA_BUF);
#endif

static struct platform_device *pdev_qdmabuf;

static int __init qdmabuf_mod_init(void)
{
	int err;

	pr_info("%s", version);

	err = qdmabuf_cdev_register();
	if (err != 0) {
		pr_err("qdmabuf_cdev_register() failed, err=%d\n", err);
		goto err0;
	}

	err = qdmabuf_device_register();
	if (err != 0) {
		pr_err("qdmabuf_device_register() failed, err=%d\n", err);
		goto err1;
	}

	pdev_qdmabuf = platform_device_alloc(QDMABUF_DRIVER_NAME, 0);
	if (pdev_qdmabuf == NULL) {
		pr_err("platform_device_alloc() failed\n");
		err = -ENOMEM;
		goto err2;
	}

	err = platform_device_add(pdev_qdmabuf);
	if (err != 0) {
		pr_err("platform_device_add() failed, err=%d\n", err);
		err = -ENOMEM;
		goto err3;
	}

	return err;

err3:
	platform_device_put(pdev_qdmabuf);
err2:
	qdmabuf_device_unregister();
err1:
	qdmabuf_cdev_unregister();
err0:
	return err;
}

static void __exit qdmabuf_mod_exit(void)
{
	pr_info("%s", version);

	platform_device_del(pdev_qdmabuf);
	platform_device_put(pdev_qdmabuf);
	qdmabuf_device_unregister();
	qdmabuf_cdev_unregister();
}

module_init(qdmabuf_mod_init);
module_exit(qdmabuf_mod_exit);
