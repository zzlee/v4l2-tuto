#ifndef __QVIO_DEVICE_H__
#define __QVIO_DEVICE_H__

#include "queue.h"

#define QVIO_DRIVER_NAME		"qvio"

struct qvio_device;

int qvio_device_register(void);
void qvio_device_unregister(void);

struct qvio_device* qvio_device_new(void);
struct qvio_device* qvio_device_get(struct qvio_device* self);
void qvio_device_put(struct qvio_device* self);

int qvio_device_start(struct qvio_device* self);
void qvio_device_stop(struct qvio_device* self);

int qvio_device_s_fmt(struct qvio_device* self, struct v4l2_format *format);
int qvio_device_g_fmt(struct qvio_device* self, struct v4l2_format *format);
int qvio_device_try_fmt(struct qvio_device* self, struct v4l2_format *format);

#endif // __QVIO_DEVICE_H__