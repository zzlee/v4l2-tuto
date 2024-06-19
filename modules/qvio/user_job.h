#ifndef __QVIO_USER_JOB_H__
#define __QVIO_USER_JOB_H__

#include "device.h"
#include "uapi/qvio.h"

void qvio_user_job_init(struct qvio_device* self);

// proprietary v4l2 ioctl
long qvio_user_job_ioctl_wait(struct qvio_device* self, unsigned long arg);
long qvio_user_job_ioctl_done(struct qvio_device* self, unsigned long arg);

// user-job
int qvio_user_job_s_fmt(struct qvio_device* self, struct v4l2_format *format);
int qvio_user_job_queue_setup(struct qvio_device* self, unsigned int num_buffers);
int qvio_user_job_buf_init(struct qvio_device* self, struct vb2_buffer *buffer);
int qvio_user_job_buf_cleanup(struct qvio_device* self, struct vb2_buffer *buffer);
int qvio_user_job_start_streaming(struct qvio_device* self);
int qvio_user_job_stop_streaming(struct qvio_device* self);
int qvio_user_job_buf_done(struct qvio_device* self, struct vb2_buffer *buffer);

#endif // __QVIO_USER_JOB_H__