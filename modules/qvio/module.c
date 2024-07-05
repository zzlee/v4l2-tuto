#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>

#include "version.h"
#include "platform_device.h"
#include "pci_device.h"

#define DRV_MODULE_DESC		"QCAP Video I/O Driver"

static char version[] = DRV_MODULE_DESC " v" DRV_MODULE_VERSION "\n";

MODULE_AUTHOR("ZzLab");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_LICENSE("GPL");

static int __init qvio_mod_init(void)
{
	int err;

	pr_info("%s", version);

#if 0
	err = qvio_device_platform_register();
	if (err != 0) {
		pr_err("qvio_device_platform_register() failed, err=%d\n", err);
		goto err0;
	}
#endif

	err = qvio_device_pci_register();
	if (err != 0) {
		pr_err("qvio_device_pci_register() failed, err=%d\n", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static void __exit qvio_mod_exit(void)
{
	pr_info("%s", version);

	qvio_device_pci_unregister();

#if 0
	qvio_device_platform_unregister();
#endif
}

module_init(qvio_mod_init);
module_exit(qvio_mod_exit);
