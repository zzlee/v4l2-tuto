#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "platform_device.h"
#include "device.h"

#include <linux/platform_device.h>

#define DRV_MODULE_NAME "qvio"

static int __platform_probe(struct platform_device *pdev) {
	int err = 0;
	struct qvio_device* self;

	pr_info("\n");

	self = qvio_device_new();
	if(! self) {
		pr_err("qvio_device_new() failed\n");
		err = -ENOMEM;
		goto err0;
	}

	self->dev = &pdev->dev;
	self->pdev = pdev;
	platform_set_drvdata(pdev, self);

	self->video[0] = qvio_video_new();
	if(! self) {
		pr_err("qvio_video_new() failed\n");
		err = -ENOMEM;
		goto err1;
	}

	self->video[0]->user_job_ctrl.enable = true;

	self->video[0]->vfl_dir = VFL_DIR_RX;
	self->video[0]->buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	self->video[0]->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	snprintf(self->video[0]->bus_info, sizeof(self->video[0]->bus_info), "platform");
	snprintf(self->video[0]->v4l2_dev.name, V4L2_DEVICE_NAME_SIZE, "qvio-rx");

	err = qvio_video_start(self->video[0]);
	if(err) {
		pr_err("qvio_qvio_start() failed, err=%d\n", err);
		goto err2;
	}

	return 0;

err2:
	qvio_video_put(self->video[0]);
err1:
	qvio_device_put(self);
err0:
	return err;
}

static int __platform_remove(struct platform_device *pdev) {
	struct qvio_device* self = platform_get_drvdata(pdev);

	pr_info("\n");

	qvio_video_stop(self->video[0]);
	qvio_video_put(self->video[0]);
	qvio_device_put(self);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct platform_driver platform_driver = {
	.driver = {
		.name = DRV_MODULE_NAME
	},
	.probe  = __platform_probe,
	.remove = __platform_remove,
};

static struct platform_device *pdev_qvio;

int qvio_device_platform_register(void) {
	int err;

	pr_info("\n");

	err = platform_driver_register(&platform_driver);
	if(err) {
		pr_err("platform_driver_register() failed\n");
		goto err0;
	}

	pdev_qvio = platform_device_alloc(DRV_MODULE_NAME, 0);
	if (pdev_qvio == NULL) {
		pr_err("platform_device_alloc() failed\n");
		err = -ENOMEM;
		goto err1;
	}

	err = platform_device_add(pdev_qvio);
	if (err != 0) {
		pr_err("platform_device_add() failed, err=%d\n", err);
		err = -ENOMEM;
		goto err2;
	}

	return 0;

err2:
	platform_device_put(pdev_qvio);
err1:
	platform_driver_unregister(&platform_driver);
err0:
	return err;
}

void qvio_device_platform_unregister(void) {
	pr_info("\n");

	platform_device_del(pdev_qvio);
	platform_device_put(pdev_qvio);
	platform_driver_unregister(&platform_driver);
}
