#ifndef __QVIO_USER_JOB_H__
#define __QVIO_USER_JOB_H__

#include <linux/wait.h>
#include <linux/videodev2.h>
#include <media/videobuf2-core.h>
#include <linux/fs.h>

#include "uapi/qvio.h"

struct qvio_user_job_dev {
	wait_queue_head_t user_job_wq;
	atomic_t user_job_wq_event;
	__u32 last_user_job_wq_event;
	atomic_t user_job_sequence;
	struct qvio_user_job current_user_job;
	wait_queue_head_t user_job_done_wq;
	atomic_t user_job_done_wq_event;
	struct qvio_user_job_done current_user_job_done;
};

void qvio_user_job_init(struct qvio_user_job_dev* self);

// proprietary v4l2 ioctl
long qvio_user_job_ioctl_get(struct qvio_user_job_dev* self, unsigned long arg);
long qvio_user_job_ioctl_done(struct qvio_user_job_dev* self, unsigned long arg);

// proprietary file ops
ssize_t qvio_user_job_read(struct qvio_user_job_dev* self, struct file *filp, char *buf, size_t size, loff_t *f_pos);
ssize_t qvio_user_job_write(struct qvio_user_job_dev* self, struct file *filp, const char *buf, size_t size, loff_t *f_pos);
__poll_t qvio_user_job_poll(struct qvio_user_job_dev* self, struct file *filp, struct poll_table_struct *wait);

// user-job
int qvio_user_job_s_fmt(struct qvio_user_job_dev* self, struct v4l2_format *format);
int qvio_user_job_queue_setup(struct qvio_user_job_dev* self, unsigned int num_buffers);
int qvio_user_job_buf_init(struct qvio_user_job_dev* self, struct vb2_buffer *buffer);
int qvio_user_job_buf_cleanup(struct qvio_user_job_dev* self, struct vb2_buffer *buffer);
int qvio_user_job_start_streaming(struct qvio_user_job_dev* self);
int qvio_user_job_stop_streaming(struct qvio_user_job_dev* self);
int qvio_user_job_buf_done(struct qvio_user_job_dev* self, struct vb2_buffer *buffer);

#endif // __QVIO_USER_JOB_H__