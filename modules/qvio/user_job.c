#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "user_job.h"

void qvio_user_job_init(struct qvio_user_job_dev* self) {
	init_waitqueue_head(&self->user_job_wq);
	atomic_set(&self->user_job_wq_event, 0);
	self->last_user_job_wq_event = 0;
	atomic_set(&self->user_job_sequence, 0);
	memset(&self->current_user_job, 0, sizeof(struct qvio_user_job));
	init_waitqueue_head(&self->user_job_done_wq);
	atomic_set(&self->user_job_done_wq_event, 0);
	memset(&self->current_user_job_done, 0, sizeof(struct qvio_user_job));
}

long qvio_user_job_ioctl_get(struct qvio_user_job_dev* self, unsigned long arg) {
	long ret;
	__u32 current_wq_event;

	current_wq_event = atomic_read(&self->user_job_wq_event);
	if (current_wq_event == self->last_user_job_wq_event) {
		pr_err("unexpected value, current_wq_event=%d\n", (int)current_wq_event);

		ret = -EFAULT;
		goto err0;
	}

	ret = copy_to_user((void __user *)arg, &self->current_user_job, sizeof(struct qvio_user_job));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	self->last_user_job_wq_event = current_wq_event;

	ret = 0;

	return ret;

err0:
	return ret;
}

long qvio_user_job_ioctl_done(struct qvio_user_job_dev* self, unsigned long arg) {
	long ret;

	ret = copy_from_user(&self->current_user_job_done, (void __user *)arg, sizeof(struct qvio_user_job_done));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	atomic_inc(&self->user_job_done_wq_event);
	wake_up_interruptible(&self->user_job_done_wq);

	ret = 0;

	return ret;

err0:
	return ret;
}

ssize_t qvio_user_job_read(struct qvio_user_job_dev* self, struct file *filp, char *buf, size_t size, loff_t *f_pos) {
	ssize_t ret;
	DECLARE_WAITQUEUE(wait, current);
	__u32 current_wq_event;

	if (size != sizeof(struct qvio_user_job))
		return -EINVAL;

	add_wait_queue(&self->user_job_wq, &wait);

	do {
		set_current_state(TASK_INTERRUPTIBLE);

		current_wq_event = atomic_read(&self->user_job_wq_event);
		if (current_wq_event != self->last_user_job_wq_event) {
			__set_current_state(TASK_RUNNING);

			ret = copy_to_user(buf, &self->current_user_job, size);
			if (ret) {
				pr_err("copy_to_user() failed, err=%d\n", (int)ret);

				ret = -EFAULT;
			} else {
				self->last_user_job_wq_event = current_wq_event;
				ret = size;
			}
			break;
		}

		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}
		schedule();
	} while (1);

	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&self->user_job_wq, &wait);

	return ret;
}

ssize_t qvio_user_job_write(struct qvio_user_job_dev* self, struct file *filp, const char *buf, size_t size, loff_t *f_pos) {
	ssize_t ret;

	if (size != sizeof(struct qvio_user_job_done)) {
		pr_err("unexpected value, size=%d\n", (int)size);
		return -EINVAL;
	}

	ret = copy_from_user(&self->current_user_job_done, buf, size);
	if (ret) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
	} else {
		ret = size;
	}

	atomic_inc(&self->user_job_done_wq_event);
	wake_up_interruptible(&self->user_job_done_wq);

	return ret;
}

__poll_t qvio_user_job_poll(struct qvio_user_job_dev* self, struct file *filp, struct poll_table_struct *wait) {
	poll_wait(filp, &self->user_job_wq, wait);
	if(self->last_user_job_wq_event != atomic_read(&self->user_job_wq_event))
		return EPOLLIN | EPOLLRDNORM;

	return 0;
}

static int __user_job_wait_done(struct qvio_user_job_dev* self) {
	int err;
	__u32 last_wq_event;

	pr_info("\n");

	atomic_inc(&self->user_job_wq_event);
	wake_up_interruptible(&self->user_job_wq);

#if 1 // DEBUG_USER_JOB
	last_wq_event = atomic_read(&self->user_job_done_wq_event);
	pr_info("last_wq_event=%d\n", (int)last_wq_event);
	err = wait_event_interruptible_timeout(self->user_job_done_wq,
		(last_wq_event != atomic_read(&self->user_job_done_wq_event)),
		msecs_to_jiffies(300));
	if(err <= 0) {
		pr_err("wait_event_interruptible() failed, err=%d\n", err);
		goto err0;
	}
#endif // DEBUG_USER_JOB

	pr_info("\n");

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_s_fmt(struct qvio_user_job_dev* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	self->current_user_job.id = QVIO_USER_JOB_ID_S_FMT;
	self->current_user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	memcpy(&self->current_user_job.u.s_fmt.format, format, sizeof(struct v4l2_format));

	err = __user_job_wait_done(self);
	if(err) {
		pr_err("__user_job_wait_done() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_queue_setup(struct qvio_user_job_dev* self, unsigned int num_buffers) {
	int err;

	pr_info("\n");

	self->current_user_job.id = QVIO_USER_JOB_ID_QUEUE_SETUP;
	self->current_user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	self->current_user_job.u.queue_setup.num_buffers = num_buffers;

	err = __user_job_wait_done(self);
	if(err) {
		pr_err("__user_job_wait_done() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_buf_init(struct qvio_user_job_dev* self, struct vb2_buffer *buffer) {
	int err;

	pr_info("\n");

	self->current_user_job.id = QVIO_USER_JOB_ID_BUF_INIT;
	self->current_user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	self->current_user_job.u.buf_init.index = buffer->index;

	err = __user_job_wait_done(self);
	if(err) {
		pr_err("__user_job_wait_done() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_buf_cleanup(struct qvio_user_job_dev* self, struct vb2_buffer *buffer) {
	int err;

	pr_info("\n");

	self->current_user_job.id = QVIO_USER_JOB_ID_BUF_CLEANUP;
	self->current_user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	self->current_user_job.u.buf_cleanup.index = buffer->index;

	err = __user_job_wait_done(self);
	if(err) {
		pr_err("__user_job_wait_done() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_start_streaming(struct qvio_user_job_dev* self) {
	int err;

	pr_info("\n");

	self->current_user_job.id = QVIO_USER_JOB_ID_START_STREAMING;
	self->current_user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	self->current_user_job.u.start_streaming.flags = 0;

	err = __user_job_wait_done(self);
	if(err) {
		pr_err("__user_job_wait_done() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_stop_streaming(struct qvio_user_job_dev* self) {
	int err;

	pr_info("\n");

	self->current_user_job.id = QVIO_USER_JOB_ID_STOP_STREAMING;
	self->current_user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	self->current_user_job.u.stop_streaming.flags = 0;

	err = __user_job_wait_done(self);
	if(err) {
		pr_err("__user_job_wait_done() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_buf_done(struct qvio_user_job_dev* self, struct vb2_buffer *buffer) {
	int err;

	pr_info("\n");

	self->current_user_job.id = QVIO_USER_JOB_ID_BUF_DONE;
	self->current_user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	self->current_user_job.u.buf_done.index = buffer->index;

	err = __user_job_wait_done(self);
	if(err) {
		pr_err("__user_job_wait_done() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	return err;

err0:
	return err;
}
