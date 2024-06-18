#ifndef __QDMABUF_CDEV_H__
#define __QDMABUF_CDEV_H__

#include "device.h"

int qdmabuf_cdev_register(void);
void qdmabuf_cdev_unregister(void);

void qdmabuf_cdev_init(struct qdmabuf_device* self);

int qdmabuf_cdev_start(struct qdmabuf_device* self);
void qdmabuf_cdev_stop(struct qdmabuf_device* self);

#endif // __QDMABUF_CDEV_H__