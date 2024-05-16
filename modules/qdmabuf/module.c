#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s: " fmt, __func__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "version.h"
#include "module.h"
#include "cdev.h"

#define DRV_MODULE_NAME		"qdmabuf"
#define DRV_MODULE_DESC		"QCAP dma-buf Driver"

static char version[] =
	DRV_MODULE_DESC " " DRV_MODULE_NAME " v" DRV_MODULE_VERSION "\n";

MODULE_AUTHOR("ZzLab");
MODULE_DESCRIPTION(DRV_MODULE_DESC);
MODULE_VERSION(DRV_MODULE_VERSION);
MODULE_LICENSE("Dual BSD/GPL");

static struct platform_device* qdmabuf_device;

static int qdmabuf_probe(struct platform_device *pdev)
{
	int err;
	struct qdmabuf_drvdata* drvdata;

	pr_info("%s(#%d)\n", __func__, __LINE__);

	drvdata = devm_kzalloc(&pdev->dev, sizeof(struct qdmabuf_drvdata), GFP_KERNEL);
	if(! drvdata) {
		pr_err("%s(#%d): devm_kzalloc() failed, drvdata=%p\n", __func__, __LINE__, drvdata);

		err = -ENOMEM;
		goto err0;
	}
	drvdata->device = &pdev->dev;

	platform_set_drvdata(pdev, drvdata);

	err = qdmabuf_cdev_init();
	if(err) {
		pr_err("%s(#%d): qdmabuf_cdev_init() failed, err=%d\n", __func__, __LINE__, err);

		goto err1;
	}

	err = qdmabuf_cdev_create_interfaces(drvdata->device);
	if(err) {
		pr_err("%s(#%d): qdmabuf_cdev_create_interfaces() failed, err=%d\n", __func__, __LINE__, err);

		goto err2;
	}

	return 0;

err2:
	qdmabuf_cdev_cleanup();
err1:
err0:

	return err;
}

static int qdmabuf_remove(struct platform_device *pdev)
{
	pr_info("%s(#%d)\n", __func__, __LINE__);

	qdmabuf_cdev_cleanup();

	return 0;
}

static struct platform_driver qdmabuf_driver = {
	.driver = {
		.name  = DRV_MODULE_NAME,
		.owner = THIS_MODULE,
	},
	.probe  = qdmabuf_probe,
	.remove = qdmabuf_remove,
};

static int qdmabuf_mod_init(void)
{
	int err;

	pr_info("%s", version);

	qdmabuf_device = platform_device_alloc(DRV_MODULE_NAME, 0);
	if (!qdmabuf_device) {
		pr_err("%s(#%d): platform_device_alloc() failed\n", __func__, __LINE__);

		err = -ENOMEM;
		goto err0;
	}

	err = platform_device_add(qdmabuf_device);
	if (err) {
		pr_err("%s(#%d): platform_device_add() failed, err=%d\n", __func__, __LINE__, err);

		goto err1;
	}

	err = platform_driver_register(&qdmabuf_driver);
	if (err) {
		pr_err("%s(#%d): platform_driver_register() failed, err=%d\n", __func__, __LINE__, err);

		goto err2;
	}

	return err;

err2:
	platform_device_unregister(qdmabuf_device);
err1:
	platform_device_put(qdmabuf_device);
	qdmabuf_device = NULL;
err0:

	return err;
}

static void qdmabuf_mod_exit(void)
{
	pr_info("%s", version);

	platform_driver_unregister(&qdmabuf_driver);
	platform_device_unregister(qdmabuf_device);
	platform_device_put(qdmabuf_device);
	qdmabuf_device = NULL;
}

module_init(qdmabuf_mod_init);
module_exit(qdmabuf_mod_exit);
