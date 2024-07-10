#ifndef __QDMABUF_DEVICE_H__
#define __QDMABUF_DEVICE_H__

#include <linux/cdev.h>

#define QDMABUF_DRIVER_NAME "qdmabuf"

struct qdmabuf_device {
	struct kref ref;
	struct platform_device* pdev;

	// cdev
	dev_t cdevno;
	struct cdev cdev;
};

int qdmabuf_device_register(void);
void qdmabuf_device_unregister(void);

struct qdmabuf_device* qdmabuf_device_new(void);
struct qdmabuf_device* qdmabuf_device_get(struct qdmabuf_device* self);
void qdmabuf_device_put(struct qdmabuf_device* self);

int qdmabuf_device_start(struct qdmabuf_device* self);
void qdmabuf_device_stop(struct qdmabuf_device* self);

#endif // __QDMABUF_DEVICE_H__