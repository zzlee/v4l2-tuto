#ifndef __QVIO_USER_JOB_H__
#define __QVIO_USER_JOB_H__

#include <linux/wait.h>
#include <linux/videodev2.h>
#include <media/videobuf2-core.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/spinlock_types.h>

#include "uapi/qvio.h"

typedef void (*qvio_user_job_done_handler)(void* user, struct qvio_user_job_done* user_job_done);

struct qvio_user_job_dev {
	atomic_t user_job_sequence;

	// user-job list
	wait_queue_head_t user_job_wq;
	spinlock_t user_job_list_lock;
	struct list_head user_job_list;

	// user-job-done lsit
	wait_queue_head_t user_job_done_wq;
	spinlock_t user_job_done_list_lock;
	struct list_head user_job_done_list;
};

void qvio_user_job_init(struct qvio_user_job_dev* self);
void qvio_user_job_uninit(struct qvio_user_job_dev* self);

// proprietary v4l2 ioctl
long qvio_user_job_ioctl_get(struct qvio_user_job_dev* self, unsigned long arg);
long qvio_user_job_ioctl_done(struct qvio_user_job_dev* self, unsigned long arg);

// proprietary file ops
__poll_t qvio_user_job_poll(struct qvio_user_job_dev* self, struct file *filp, struct poll_table_struct *wait);

// user-job
int qvio_user_job_s_fmt(struct qvio_user_job_dev* self, struct v4l2_format *format);
int qvio_user_job_queue_setup(struct qvio_user_job_dev* self, unsigned int num_buffers);
int qvio_user_job_buf_init(struct qvio_user_job_dev* self, struct vb2_buffer *buffer, void* user, qvio_user_job_done_handler fn);
int qvio_user_job_buf_cleanup(struct qvio_user_job_dev* self, struct vb2_buffer *buffer);
int qvio_user_job_start_streaming(struct qvio_user_job_dev* self);
int qvio_user_job_stop_streaming(struct qvio_user_job_dev* self);
int qvio_user_job_buf_done(struct qvio_user_job_dev* self, struct vb2_buffer *buffer);

#endif // __QVIO_USER_JOB_H__