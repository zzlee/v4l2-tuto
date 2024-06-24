#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "user_job.h"

struct __user_job_entry {
	struct list_head node;
	struct qvio_user_job user_job;
};

struct __user_job_done_entry {
	struct list_head node;
	struct qvio_user_job_done user_job_done;
};

int __user_job_entry_new(struct __user_job_entry** user_job_entry) {
	int err;

	*user_job_entry = kzalloc(sizeof(struct __user_job_entry), GFP_KERNEL);
	if(! *user_job_entry) {
		pr_err("out of memory\n");

		err = ENOMEM;
		goto err0;
	}
	INIT_LIST_HEAD(&(*user_job_entry)->node);

	err = 0;

	return err;

err0:
	return err;
}

int __user_job_done_entry_new(struct __user_job_done_entry** user_job_done_entry) {
	int err;

	*user_job_done_entry = kzalloc(sizeof(struct __user_job_done_entry), GFP_KERNEL);
	if(! *user_job_done_entry) {
		pr_err("out of memory\n");

		err = ENOMEM;
		goto err0;
	}
	INIT_LIST_HEAD(&(*user_job_done_entry)->node);

	err = 0;

	return err;

err0:
	return err;
}

void qvio_user_job_init(struct qvio_user_job_dev* self) {
	atomic_set(&self->user_job_sequence, 0);

	init_waitqueue_head(&self->user_job_wq);
	spin_lock_init(&self->user_job_list_lock);
	INIT_LIST_HEAD(&self->user_job_list);

	init_waitqueue_head(&self->user_job_done_wq);
	spin_lock_init(&self->user_job_done_list_lock);
	INIT_LIST_HEAD(&self->user_job_done_list);
}

void qvio_user_job_uninit(struct qvio_user_job_dev* self) {
	unsigned long flags;
	struct __user_job_entry *user_job_entry;
	struct __user_job_done_entry *user_job_done_entry;

	pr_info("\n");

	if(! list_empty(&self->user_job_list)) {
		pr_warn("self->user_job_list is not empty\n");

		spin_lock_irqsave(&self->user_job_list_lock, flags);
		list_for_each_entry(user_job_entry, &self->user_job_list, node) {
			list_del(&user_job_entry->node);
			kfree(user_job_entry);
		}
		spin_unlock_irqrestore(&self->user_job_list_lock, flags);
	}

	if(! list_empty(&self->user_job_done_list)) {
		pr_warn("self->user_job_done_list is not empty\n");

		spin_lock_irqsave(&self->user_job_done_list_lock, flags);
		list_for_each_entry(user_job_done_entry, &self->user_job_done_list, node) {
			list_del(&user_job_done_entry->node);
			kfree(user_job_done_entry);
		}
		spin_unlock_irqrestore(&self->user_job_done_list_lock, flags);
	}
}

long qvio_user_job_ioctl_get(struct qvio_user_job_dev* self, unsigned long arg) {
	long ret;
	struct __user_job_entry *user_job_entry;
	unsigned long flags;

	if(list_empty(&self->user_job_list)) {
		pr_err("unexpected value, self->user_job_list is empty\n");

		ret = -EFAULT;
		goto err0;
	}

	spin_lock_irqsave(&self->user_job_list_lock, flags);
	user_job_entry = list_first_entry(&self->user_job_list, struct __user_job_entry, node);
	list_del(&user_job_entry->node);
	spin_unlock_irqrestore(&self->user_job_list_lock, flags);

	ret = copy_to_user((void __user *)arg, &user_job_entry->user_job, sizeof(struct qvio_user_job));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err1;
	}

	kfree(user_job_entry);
	ret = 0;

	return ret;

err1:
	kfree(user_job_entry);
err0:
	return ret;
}

long qvio_user_job_ioctl_done(struct qvio_user_job_dev* self, unsigned long arg) {
	int err;
	long ret;
	struct __user_job_done_entry *user_job_done_entry;
	unsigned long flags;

	err = __user_job_done_entry_new(&user_job_done_entry);
	if(err) {
		pr_err("__user_job_done_entry_new() failed, err=%d\n", err);

		ret = -err;
		goto err0;
	}

	ret = copy_from_user(&user_job_done_entry->user_job_done, (void __user *)arg, sizeof(struct qvio_user_job_done));
	if (ret != 0) {
		pr_err("copy_from_user() failed, err=%d\n", (int)ret);

		ret = -EFAULT;
		goto err1;
	}

	spin_lock_irqsave(&self->user_job_done_list_lock, flags);
	list_add_tail(&user_job_done_entry->node, &self->user_job_done_list);
	spin_unlock_irqrestore(&self->user_job_done_list_lock, flags);

	wake_up_interruptible(&self->user_job_done_wq);

	ret = 0;

	return ret;

err1:
	kfree(user_job_done_entry);
err0:
	return ret;
}

__poll_t qvio_user_job_poll(struct qvio_user_job_dev* self, struct file *filp, struct poll_table_struct *wait) {
	poll_wait(filp, &self->user_job_wq, wait);

	if(!list_empty(&self->user_job_list))
		return EPOLLIN | EPOLLRDNORM;

	return 0;
}

static void __do_user_job(struct qvio_user_job_dev* self, struct __user_job_entry* user_job_entry) {
	unsigned long flags;

	pr_info("\n");

	spin_lock_irqsave(&self->user_job_list_lock, flags);
	list_add_tail(&user_job_entry->node, &self->user_job_list);
	spin_unlock_irqrestore(&self->user_job_list_lock, flags);
	wake_up_interruptible(&self->user_job_wq);
}

typedef void (*user_job_done_handler)(void* user, struct __user_job_done_entry* user_job_done_entry);

static int __wait_for_user_job_done(struct qvio_user_job_dev* self, void* user, user_job_done_handler fn) {
	int err;
	unsigned long flags;
	struct __user_job_done_entry* user_job_done_entry;

	pr_info("\n");

	// wait for user-job-done
	err = wait_event_interruptible(self->user_job_done_wq, !list_empty(&self->user_job_done_list));
	if(err != 0) {
		pr_err("wait_event_interruptible() failed, err=%d\n", err);
		goto err0;
	}

	err = 0;

	pr_info("\n");

	if(list_empty(&self->user_job_done_list)) {
		pr_err("self->user_job_done_list is empty\n");
		err = EFAULT;
		goto err0;
	}

	spin_lock_irqsave(&self->user_job_done_list_lock, flags);
	user_job_done_entry = list_first_entry(&self->user_job_done_list, struct __user_job_done_entry, node);
	list_del(&user_job_done_entry->node);
	spin_unlock_irqrestore(&self->user_job_done_list_lock, flags);

	if(fn) {
		fn(user, user_job_done_entry);
	}
	kfree(user_job_done_entry);

	return err;

err0:
	return err;
}

int qvio_user_job_s_fmt(struct qvio_user_job_dev* self, struct v4l2_format *format) {
	int err;
	struct __user_job_entry *user_job_entry;

	pr_info("\n");

	err = __user_job_entry_new(&user_job_entry);
	if(err) {
		pr_err("__user_job_entry_new() failed, err=%d\n", err);
		goto err0;
	}

	user_job_entry->user_job.id = QVIO_USER_JOB_ID_S_FMT;
	user_job_entry->user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	memcpy(&user_job_entry->user_job.u.s_fmt.format, format, sizeof(struct v4l2_format));
	__do_user_job(self, user_job_entry);

#if 1 // USER_JOB_NOACK
	err = __wait_for_user_job_done(self, NULL, NULL);
	if(err) {
		pr_err("__wait_for_user_job_done() failed, err=%d\n", err);
		goto err0;
	}
#endif // USER_JOB_NOACK

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_queue_setup(struct qvio_user_job_dev* self, unsigned int num_buffers) {
	int err;
	struct __user_job_entry *user_job_entry;

	pr_info("\n");

	err = __user_job_entry_new(&user_job_entry);
	if(err) {
		pr_err("__user_job_entry_new() failed, err=%d\n", err);
		goto err0;
	}

	user_job_entry->user_job.id = QVIO_USER_JOB_ID_QUEUE_SETUP;
	user_job_entry->user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	user_job_entry->user_job.u.queue_setup.num_buffers = num_buffers;
	__do_user_job(self, user_job_entry);

#if 1 // USER_JOB_NOACK
	err = __wait_for_user_job_done(self, NULL, NULL);
	if(err) {
		pr_err("__wait_for_user_job_done() failed, err=%d\n", err);
		goto err0;
	}
#endif // USER_JOB_NOACK

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_buf_init(struct qvio_user_job_dev* self, struct vb2_buffer *buffer) {
	int err;
	struct __user_job_entry *user_job_entry;

	pr_info("\n");

	err = __user_job_entry_new(&user_job_entry);
	if(err) {
		pr_err("__user_job_entry_new() failed, err=%d\n", err);
		goto err0;
	}

	user_job_entry->user_job.id = QVIO_USER_JOB_ID_BUF_INIT;
	user_job_entry->user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	user_job_entry->user_job.u.buf_init.index = buffer->index;
	__do_user_job(self, user_job_entry);

#if 1 // USER_JOB_NOACK
	err = __wait_for_user_job_done(self, NULL, NULL);
	if(err) {
		pr_err("__wait_for_user_job_done() failed, err=%d\n", err);
		goto err0;
	}
#endif // USER_JOB_NOACK

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_buf_cleanup(struct qvio_user_job_dev* self, struct vb2_buffer *buffer) {
	int err;
	struct __user_job_entry *user_job_entry;

	pr_info("\n");

	err = __user_job_entry_new(&user_job_entry);
	if(err) {
		pr_err("__user_job_entry_new() failed, err=%d\n", err);
		goto err0;
	}

	user_job_entry->user_job.id = QVIO_USER_JOB_ID_BUF_CLEANUP;
	user_job_entry->user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	user_job_entry->user_job.u.buf_cleanup.index = buffer->index;
	__do_user_job(self, user_job_entry);

#if 1 // USER_JOB_NOACK
	err = __wait_for_user_job_done(self, NULL, NULL);
	if(err) {
		pr_err("__wait_for_user_job_done() failed, err=%d\n", err);
		goto err0;
	}
#endif // USER_JOB_NOACK

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_start_streaming(struct qvio_user_job_dev* self) {
	int err;
	struct __user_job_entry *user_job_entry;

	pr_info("\n");

	err = __user_job_entry_new(&user_job_entry);
	if(err) {
		pr_err("__user_job_entry_new() failed, err=%d\n", err);
		goto err0;
	}

	user_job_entry->user_job.id = QVIO_USER_JOB_ID_START_STREAMING;
	user_job_entry->user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	user_job_entry->user_job.u.start_streaming.flags = 0;
	__do_user_job(self, user_job_entry);

#if 1 // USER_JOB_NOACK
	err = __wait_for_user_job_done(self, NULL, NULL);
	if(err) {
		pr_err("__wait_for_user_job_done() failed, err=%d\n", err);
		goto err0;
	}
#endif // USER_JOB_NOACK

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_stop_streaming(struct qvio_user_job_dev* self) {
	int err;
	struct __user_job_entry *user_job_entry;

	pr_info("\n");

	err = __user_job_entry_new(&user_job_entry);
	if(err) {
		pr_err("__user_job_entry_new() failed, err=%d\n", err);
		goto err0;
	}

	user_job_entry->user_job.id = QVIO_USER_JOB_ID_STOP_STREAMING;
	user_job_entry->user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	user_job_entry->user_job.u.stop_streaming.flags = 0;
	__do_user_job(self, user_job_entry);

#if 1 // USER_JOB_NOACK
	err = __wait_for_user_job_done(self, NULL, NULL);
	if(err) {
		pr_err("__wait_for_user_job_done() failed, err=%d\n", err);
		goto err0;
	}
#endif // USER_JOB_NOACK

	err = 0;

	return err;

err0:
	return err;
}

int qvio_user_job_buf_done(struct qvio_user_job_dev* self, struct vb2_buffer *buffer) {
	int err;
	struct __user_job_entry *user_job_entry;

	pr_info("\n");

	err = __user_job_entry_new(&user_job_entry);
	if(err) {
		pr_err("__user_job_entry_new() failed, err=%d\n", err);
		goto err0;
	}

	user_job_entry->user_job.id = QVIO_USER_JOB_ID_BUF_DONE;
	user_job_entry->user_job.sequence = (__u16)atomic_inc_return(&self->user_job_sequence);
	user_job_entry->user_job.u.buf_done.index = buffer->index;
	__do_user_job(self, user_job_entry);

#if 1 // USER_JOB_NOACK
	err = __wait_for_user_job_done(self, NULL, NULL);
	if(err) {
		pr_err("__wait_for_user_job_done() failed, err=%d\n", err);
		goto err0;
	}
#endif // USER_JOB_NOACK

	err = 0;

	return err;

err0:
	return err;
}
