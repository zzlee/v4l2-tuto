#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "version.h"
#include "module.h"
#include "device.h"

#define DRV_MODULE_NAME		"qvio"
#define DRV_MODULE_DESC		"QCAP Video I/O Driver"

static char version[] =
	DRV_MODULE_DESC " " DRV_MODULE_NAME " v" DRV_MODULE_VERSION "\n";

MODULE_AUTHOR("ZzLab");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_LICENSE("GPL");

static struct platform_device* g_pdev;
static struct qvio_device* g_dev;

static int qvio_probe(struct platform_device *pdev)
{
	int err;
	struct qvio_drvdata* drvdata;

	pr_info("\n");

	drvdata = devm_kzalloc(&pdev->dev, sizeof(struct qvio_drvdata), GFP_KERNEL);
	if(! drvdata) {
		pr_err("devm_kzalloc() failed, drvdata=%p\n", drvdata);

		err = -ENOMEM;
		goto err0;
	}
	drvdata->device = &pdev->dev;

	platform_set_drvdata(pdev, drvdata);

	g_dev = qvio_device_new();
	err = qvio_device_start(g_dev);

	return 0;

err0:
	return err;
}

static int qvio_remove(struct platform_device *pdev)
{
	pr_info("\n");

	qvio_device_put(g_dev);

	return 0;
}

static struct platform_driver qvio_driver = {
	.driver = {
		.name  = DRV_MODULE_NAME,
		.owner = THIS_MODULE,
	},
	.probe  = qvio_probe,
	.remove = qvio_remove,
};

static int qvio_mod_init(void)
{
	int err;

	pr_info("%s", version);

	g_pdev = platform_device_alloc(DRV_MODULE_NAME, 0);
	if (!g_pdev) {
		pr_err("platform_device_alloc() failed\n");

		err = -ENOMEM;
		goto err0;
	}

	err = platform_device_add(g_pdev);
	if (err) {
		pr_err("platform_device_add() failed, err=%d\n", err);

		goto err1;
	}

	err = platform_driver_register(&qvio_driver);
	if (err) {
		pr_err("platform_driver_register() failed, err=%d\n", err);

		goto err2;
	}

	return err;

err2:
	platform_device_unregister(g_pdev);
err1:
	platform_device_put(g_pdev);
	g_pdev = NULL;
err0:

	return err;
}

static void qvio_mod_exit(void)
{
	pr_info("%s", version);

	platform_driver_unregister(&qvio_driver);
	platform_device_unregister(g_pdev);
	platform_device_put(g_pdev);
	g_pdev = NULL;
}

module_init(qvio_mod_init);
module_exit(qvio_mod_exit);
