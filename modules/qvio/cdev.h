#ifndef __QVIO_CDEV_H__
#define __QVIO_CDEV_H__

#include "device.h"

int qvio_cdev_register(void);
void qvio_cdev_unregister(void);

int qvio_cdev_start(struct qvio_device* self);
void qvio_cdev_stop(struct qvio_device* self);

#endif // __QVIO_CDEV_H__