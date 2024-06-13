#ifndef __QVIO_QUEUE_H__
#define __QVIO_QUEUE_H__

#include <media/videobuf2-core.h>
#include <linux/videodev2.h>

struct qvio_queue;

struct qvio_queue* qvio_queue_new(void);
struct qvio_queue* qvio_queue_get(struct qvio_queue* self);
void qvio_queue_put(struct qvio_queue* self);

int qvio_queue_start(struct qvio_queue* self, enum v4l2_buf_type type);
void qvio_queue_stop(struct qvio_queue* self);

struct vb2_queue* qvio_queue_get_vb2_queue(struct qvio_queue* self);
int qvio_queue_s_fmt(struct qvio_queue* self, struct v4l2_format *format);
int qvio_queue_g_fmt(struct qvio_queue* self, struct v4l2_format *format);

#endif // __QVIO_QUEUE_H__