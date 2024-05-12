#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

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
		dev_err(&(pdev->dev), "%s(#%d): devm_kzalloc() fail\n", __func__, __LINE__);
		err = -ENOMEM;
		goto drvdata_alloc_failed;
	}
	drvdata->device = &pdev->dev;

	platform_set_drvdata(pdev, drvdata);

	err = qdmabuf_cdev_init();
	if(! drvdata) {
		dev_err(&(pdev->dev), "%s(#%d): qdmabuf_cdev_init() fail, err=%d\n", __func__, __LINE__, err);
		goto cdec_init_failed;
	}

	err = qdmabuf_cdev_create_interfaces(drvdata->device);
	if(! drvdata) {
		dev_err(&(pdev->dev), "%s(#%d): qdmabuf_create_interfaces() fail, err=%d\n", __func__, __LINE__, err);
		goto cdev_create_interfaces_failed;
	}

	return 0;

cdev_create_interfaces_failed:
	qdmabuf_cdev_cleanup();
cdec_init_failed:

drvdata_alloc_failed:

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
		pr_err("%s(#%d): platform_device_alloc() fail\n", __func__, __LINE__);
		err = -ENOMEM;
		goto dev_alloc_failed;
	}

	err = platform_device_add(qdmabuf_device);
	if (err) {
		pr_err("%s(#%d): platform_device_add() failed, err=%d\n",
		   __func__, __LINE__, err);
		goto dev_add_failed;
	}

	err = platform_driver_register(&qdmabuf_driver);
	if (err) {
		dev_err(&(qdmabuf_device->dev), "%s(#%d): platform_driver_register() fail, err=%d\n", __func__, __LINE__, err);
		goto drv_reg_failed;
	}

	return err;

drv_reg_failed:
	platform_device_unregister(qdmabuf_device);
dev_add_failed:
	platform_device_put(qdmabuf_device);
dev_alloc_failed:

	return err;
}

static void qdmabuf_mod_exit(void)
{
	pr_info("%s", version);

	platform_driver_unregister(&qdmabuf_driver);
	platform_device_unregister(qdmabuf_device);
}

module_init(qdmabuf_mod_init);
module_exit(qdmabuf_mod_exit);
