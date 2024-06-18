#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "device.h"
#include "cdev.h"

#include <linux/platform_device.h>
#include <linux/slab.h>

static struct qdmabuf_device* g_dev_dmabuf;

static int probe(struct platform_device *pdev) {
	int err = 0;

	pr_info("\n");

	g_dev_dmabuf = qdmabuf_device_new();
	if(! g_dev_dmabuf) {
		pr_err("qdmabuf_device_new() failed\n");
		err = -ENOMEM;
		goto err0;
	}

	g_dev_dmabuf->pdev = pdev;

	err = qdmabuf_cdev_start(g_dev_dmabuf);
	if(err) {
		pr_err("qdmabuf_cdev_start() failed, err=%d\n", err);
		goto err1;
	}

	err = qdmabuf_device_start(g_dev_dmabuf);
	if(err) {
		pr_err("qdmabuf_device_start() failed, err=%d\n", err);
		goto err2;
	}

	return err;

err2:
	qdmabuf_cdev_stop(g_dev_dmabuf);
err1:
	qdmabuf_device_put(g_dev_dmabuf);
err0:
	return err;
}

static int remove(struct platform_device *pdev) {
	int err = 0;

	pr_info("\n");

	qdmabuf_device_stop(g_dev_dmabuf);
	qdmabuf_cdev_stop(g_dev_dmabuf);
	qdmabuf_device_put(g_dev_dmabuf);

	return err;
}

static struct platform_driver qdmabuf_driver = {
	.driver = {
		.name = QDMABUF_DRIVER_NAME
	},
	.probe  = probe,
	.remove = remove,
};

int qdmabuf_device_register(void) {
	int err;

	pr_info("\n");

	err = platform_driver_register(&qdmabuf_driver);
	if(err) {
		pr_err("platform_driver_register() failed\n");
		goto err0;
	}

	return err;

err0:
	return err;
}

void qdmabuf_device_unregister(void) {
	pr_info("\n");

	platform_driver_unregister(&qdmabuf_driver);
}

struct qdmabuf_device* qdmabuf_device_new(void) {
	struct qdmabuf_device* self = kzalloc(sizeof(struct qdmabuf_device), GFP_KERNEL);

	kref_init(&self->ref);
	qdmabuf_cdev_init(self);
	init_waitqueue_head(&self->wq_head);
	self->wq_event = 0;

	return self;
}

struct qdmabuf_device* qdmabuf_device_get(struct qdmabuf_device* self) {
	if (self)
		kref_get(&self->ref);

	return self;
}

static void qdmabuf_device_free(struct kref *ref)
{
	struct qdmabuf_device* self = container_of(ref, struct qdmabuf_device, ref);

	pr_info("\n");

	kfree(self);
}

void qdmabuf_device_put(struct qdmabuf_device* self) {
	if (self)
		kref_put(&self->ref, qdmabuf_device_free);
}

int qdmabuf_device_start(struct qdmabuf_device* self) {
	int err;

	pr_info("\n");
	err = 0;

	return err;
}

void qdmabuf_device_stop(struct qdmabuf_device* self) {
	pr_info("\n");
}
