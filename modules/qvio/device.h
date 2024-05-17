#ifndef __QVIO_DEVICE_H__
#define __QVIO_DEVICE_H__

#include "queue.h"

struct qvio_device;

struct qvio_device* qvio_device_new(void);
struct qvio_device* qvio_device_get(struct qvio_device* self);
void qvio_device_put(struct qvio_device* self);

int qvio_device_start(struct qvio_device* self);
void qvio_device_stop(struct qvio_device* self);

#endif // __QVIO_DEVICE_H__