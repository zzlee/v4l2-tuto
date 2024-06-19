#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "user_job.h"

void qvio_user_job_init(struct qvio_device* self) {
	self->user_job_waiting = false;
	init_waitqueue_head(&self->user_job_wq);
	self->user_job_wq_event = 0;
	self->user_job_sequence = 0;
	memset(&self->current_user_job, 0, sizeof(struct qvio_user_job));
	init_waitqueue_head(&self->user_job_done_wq);
	self->user_job_done_wq_event = 0;
	memset(&self->current_user_job_done, 0, sizeof(struct qvio_user_job));
}

static long __user_job_wait(struct qvio_device* self, struct qvio_user_job* user_job) {
	long ret;
	int err;
	__u32 last_wq_event;

	pr_info("\n");

	self->user_job_waiting = true;

	last_wq_event = self->user_job_wq_event;
	err = wait_event_interruptible(self->user_job_wq, (last_wq_event != self->user_job_wq_event));
	if(err) {
		self->user_job_waiting = false;

		pr_err("wait_event_interruptible() failed, err=%d\n", err);
		ret = err;

		goto err0;
	}

	*user_job = self->current_user_job;
	ret = 0;

	return ret;

err0:
	return ret;
}

static long __user_job_done(struct qvio_device* self, struct qvio_user_job_done_args* args) {
	pr_info("\n");

	self->user_job_waiting = false;
	self->current_user_job_done = args->user_job_done;

	self->user_job_done_wq_event++;
	wake_up_interruptible(&self->user_job_done_wq);

	return __user_job_wait(self, &args->next_user_job);
}

long qvio_user_job_ioctl_wait(struct qvio_device* self, unsigned long arg) {
	long ret;
	struct qvio_user_job_args args;

	ret = __user_job_wait(self, &args.user_job);
	if (ret != 0) {
		pr_err("__user_job_wait() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	ret = copy_to_user((void __user *)arg, &args, sizeof(args));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	ret = 0;

	return ret;

err0:
	return ret;
}

long qvio_user_job_ioctl_done(struct qvio_device* self, unsigned long arg) {
	long ret;
	struct qvio_user_job_done_args args;

	ret = copy_from_user(&args, (void __user *)arg, sizeof(args));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	ret = __user_job_done(self, &args);
	if (ret != 0) {
		pr_err("__user_job_done() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	ret = copy_to_user((void __user *)arg, &args, sizeof(args));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err0;
	}

	ret = 0;
	return ret;

err0:
	return ret;
}

static int __user_job_wait_done(struct qvio_device* self) {
	int err;
	__u32 last_wq_event;

	pr_info("\n");

	self->user_job_wq_event++;
	wake_up_interruptible(&self->user_job_wq);

	last_wq_event = self->user_job_done_wq_event;
	err = wait_event_interruptible(self->user_job_done_wq, (last_wq_event != self->user_job_done_wq_event));
	if(err) {
		pr_err("wait_event_interruptible() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_s_fmt(struct qvio_device* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	if(! self->user_job_waiting) {
		pr_err("unexpected value, self->user_job_waiting=%d\n", (int)self->user_job_waiting);
		err = -EFAULT;
		goto err0;
	}

	self->current_user_job.id = QVIO_USER_JOB_ID_S_FMT;
	self->current_user_job.sequence = self->user_job_sequence++;
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

int qvio_user_job_queue_setup(struct qvio_device* self, unsigned int num_buffers) {
	int err;

	pr_info("\n");

	if(! self->user_job_waiting) {
		pr_err("unexpected value, self->user_job_waiting=%d\n", (int)self->user_job_waiting);
		err = -EFAULT;
		goto err0;
	}

	self->current_user_job.id = QVIO_USER_JOB_ID_QUEUE_SETUP;
	self->current_user_job.sequence = self->user_job_sequence++;
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

int qvio_user_job_buf_init(struct qvio_device* self, struct vb2_buffer *buffer) {
	int err;

	pr_info("\n");

	if(! self->user_job_waiting) {
		pr_err("unexpected value, self->user_job_waiting=%d\n", (int)self->user_job_waiting);
		err = -EFAULT;
		goto err0;
	}

	self->current_user_job.id = QVIO_USER_JOB_ID_BUF_INIT;
	self->current_user_job.sequence = self->user_job_sequence++;
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

int qvio_user_job_buf_cleanup(struct qvio_device* self, struct vb2_buffer *buffer) {
	int err;

	pr_info("\n");

	if(! self->user_job_waiting) {
		pr_err("unexpected value, self->user_job_waiting=%d\n", (int)self->user_job_waiting);
		err = -EFAULT;
		goto err0;
	}

	self->current_user_job.id = QVIO_USER_JOB_ID_BUF_CLEANUP;
	self->current_user_job.sequence = self->user_job_sequence++;
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

int qvio_user_job_start_streaming(struct qvio_device* self) {
	int err;

	pr_info("\n");

	if(! self->user_job_waiting) {
		pr_err("unexpected value, self->user_job_waiting=%d\n", (int)self->user_job_waiting);
		err = -EFAULT;
		goto err0;
	}

	self->current_user_job.id = QVIO_USER_JOB_ID_START_STREAMING;
	self->current_user_job.sequence = self->user_job_sequence++;
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

int qvio_user_job_stop_streaming(struct qvio_device* self) {
	int err;

	pr_info("\n");

	if(! self->user_job_waiting) {
		pr_err("unexpected value, self->user_job_waiting=%d\n", (int)self->user_job_waiting);
		err = -EFAULT;
		goto err0;
	}

	self->current_user_job.id = QVIO_USER_JOB_ID_STOP_STREAMING;
	self->current_user_job.sequence = self->user_job_sequence++;
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

int qvio_user_job_buf_done(struct qvio_device* self, struct vb2_buffer *buffer) {
	int err;

	pr_info("\n");

	if(! self->user_job_waiting) {
		pr_err("unexpected value, self->user_job_waiting=%d\n", (int)self->user_job_waiting);
		err = -EFAULT;
		goto err0;
	}

	self->current_user_job.id = QVIO_USER_JOB_ID_BUF_DONE;
	self->current_user_job.sequence = self->user_job_sequence++;
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
