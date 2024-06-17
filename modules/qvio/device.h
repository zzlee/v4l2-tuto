#ifndef __QVIO_DEVICE_H__
#define __QVIO_DEVICE_H__

#include "queue.h"
#include "uapi/qvio.h"

#include <media/v4l2-device.h>

#define QVIO_DRIVER_NAME		"qvio"

struct qvio_device {
	struct kref ref;

	// v4l2
	struct qvio_queue queue;
	struct mutex device_mutex;
	struct v4l2_device v4l2_dev;
	struct video_device *vdev;
	int vfl_dir;
	int videonr;
	enum v4l2_buf_type buffer_type;
	struct v4l2_format current_format;

	// cdev
	dev_t cdevno;
	struct cdev cdev;

	// user job
	bool user_job_waiting;
	wait_queue_head_t user_job_wq;
	__u32 user_job_wq_event;
	__u16 user_job_sequence;
	struct qvio_user_job current_user_job;
	wait_queue_head_t user_job_done_wq;
	__u32 user_job_done_wq_event;
	struct qvio_user_job_done current_user_job_done;
};

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

// proprietary v4l2 ioctl
long qvio_device_buf_done(struct qvio_device* self);

#endif // __QVIO_DEVICE_H__