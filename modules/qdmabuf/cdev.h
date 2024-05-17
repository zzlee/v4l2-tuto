#ifndef __QDMABUF_CDEV_H__
#define __QDMABUF_CDEV_H__

#include <linux/cdev.h>

struct qdmabuf_cdev {
	dev_t cdevno;
	struct cdev cdev;
	struct device* device; // platform device
};

int qdmabuf_cdev_init(void);
void qdmabuf_cdev_cleanup(void);
int qdmabuf_cdev_create_interfaces(struct device* device);

#endif // __QDMABUF_CDEV_H__