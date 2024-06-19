#ifndef __QVIO_CDEV_H__
#define __QVIO_CDEV_H__

#include "device.h"

#define QVIO_CDEV_NAME "qvio"

int qvio_cdev_register(void);
void qvio_cdev_unregister(void);

void qvio_cdev_init(struct qvio_device* self);

int qvio_cdev_start(struct qvio_device* self);
void qvio_cdev_stop(struct qvio_device* self);

#endif // __QVIO_CDEV_H__