#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "version.h"
#include "device.h"

#define DRV_MODULE_DESC		"QCAP Video I/O Driver"

static char version[] = DRV_MODULE_DESC " v" DRV_MODULE_VERSION "\n";

MODULE_AUTHOR("ZzLab");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_LICENSE("GPL");

static struct platform_device *pdev_qvio;

static int __init qvio_mod_init(void)
{
	int err;

	pr_info("%s", version);

	err = qvio_device_register();
	if (err != 0) {
		pr_err("qvio_device_register() failed, err=%d\n", err);
		goto err1;
	}

	pdev_qvio = platform_device_alloc(QVIO_DRIVER_NAME, 0);
	if (pdev_qvio == NULL) {
		pr_err("platform_device_alloc() failed\n");
		err = -ENOMEM;
		goto err2;
	}

	err = platform_device_add(pdev_qvio);
	if (err != 0) {
		pr_err("platform_device_add() failed, err=%d\n", err);
		err = -ENOMEM;
		goto err3;
	}

	return err;

err3:
	platform_device_put(pdev_qvio);
err2:
	qvio_device_unregister();
err1:
	return err;
}

static void __exit qvio_mod_exit(void)
{
	pr_info("%s", version);

	platform_device_del(pdev_qvio);
	platform_device_put(pdev_qvio);
	qvio_device_unregister();
}

module_init(qvio_mod_init);
module_exit(qvio_mod_exit);
