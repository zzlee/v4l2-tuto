#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "queue.h"
#include "device.h"
#include "user_job.h"

#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

struct qvio_queue_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
	int dma_buf;
};

void qvio_queue_init(struct qvio_queue* self) {
	kref_init(&self->ref);
	mutex_init(&self->queue_mutex);
	INIT_LIST_HEAD(&self->buffers);
	mutex_init(&self->buffers_mutex);
}

static int __queue_setup(struct vb2_queue *queue,
	unsigned int *num_buffers,
	unsigned int *num_planes,
	unsigned int sizes[],
	struct device *alloc_devs[]) {
	int err = 0;
	struct qvio_queue* self = container_of(queue, struct qvio_queue, queue);
	struct qvio_device* device = container_of(self, struct qvio_device, queue);

	pr_info("+param %d %d\n", *num_buffers, *num_planes);

	if(*num_buffers < 1)
		*num_buffers = 1;

	switch(self->current_format.type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		switch(self->current_format.fmt.pix.pixelformat) {
		case V4L2_PIX_FMT_NV12:
			*num_planes = 1;
			sizes[0] = ZZ_ALIGN(self->current_format.fmt.pix.width, HALIGN) * self->current_format.fmt.pix.height * 3 / 2;
			break;

		default:
			pr_err("invalid value, self->current_format.fmt.pix.pixelformat=%d", (int)self->current_format.fmt.pix.pixelformat);
			err = -EINVAL;
			goto err0;
			break;
		}
		break;

	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		switch(self->current_format.fmt.pix_mp.pixelformat) {
		case V4L2_PIX_FMT_NV12:
			*num_planes = 2;
			sizes[0] = ZZ_ALIGN(self->current_format.fmt.pix_mp.width, HALIGN) * self->current_format.fmt.pix_mp.height;
			sizes[1] = ZZ_ALIGN(self->current_format.fmt.pix_mp.width, HALIGN) * self->current_format.fmt.pix_mp.height / 2;
			break;

		default:
			pr_err("invalid value, self->current_format.fmt.pix_mp.pixelformat=%d", (int)self->current_format.fmt.pix_mp.pixelformat);
			err = -EINVAL;
			goto err0;
			break;
		}
		break;

	default:
		pr_err("unexpected value, self->current_format.type=%d\n", (int)self->current_format.type);
		err = -EINVAL;
		goto err0;
		break;
	}

	err = qvio_user_job_queue_setup(&device->user_job_ctrl, *num_buffers);
	if(err) {
		pr_warn("qvio_user_job_queue_setup() failed, err=%d", err);
	}
	err = 0;

	pr_info("-param %d %d [%d %d]\n", *num_buffers, *num_planes, sizes[0], sizes[1]);

	return err;

err0:
	return err;
}

static void __buf_init_done(void* user, struct qvio_user_job_done* user_job_done) {
	struct vb2_buffer *buffer = user;
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	struct qvio_queue_buffer* buf = container_of(vbuf, struct qvio_queue_buffer, vb);

	pr_info("BUF[%d]: dma_buf=%d offset=%d/%d pitch=%d/%d psize=%d/%d",
		vbuf->vb2_buf.index, user_job_done->u.buf_init.dma_buf,
		user_job_done->u.buf_init.offset[0], user_job_done->u.buf_init.offset[1],
		user_job_done->u.buf_init.pitch[0], user_job_done->u.buf_init.pitch[1],
		user_job_done->u.buf_init.psize[0], user_job_done->u.buf_init.psize[1]);
}

static int __buf_init(struct vb2_buffer *buffer) {
	int err;
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct qvio_device* device = container_of(self, struct qvio_device, queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	struct qvio_queue_buffer* buf = container_of(vbuf, struct qvio_queue_buffer, vb);

	pr_info("param: %p %p %d %p\n", self, vbuf, vbuf->vb2_buf.index, buf);

	err = qvio_user_job_buf_init(&device->user_job_ctrl, buffer, buffer, __buf_init_done);
	if(err) {
		pr_warn("qvio_user_job_buf_init() failed, err=%d", err);
	}
	err = 0;

	return err;
}

static void __buf_cleanup(struct vb2_buffer *buffer) {
	int err;
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct qvio_device* device = container_of(self, struct qvio_device, queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	struct qvio_queue_buffer* buf = container_of(vbuf, struct qvio_queue_buffer, vb);

	pr_info("param: %p %p %d %p\n", self, vbuf, vbuf->vb2_buf.index, buf);

	err = qvio_user_job_buf_cleanup(&device->user_job_ctrl, buffer);
	if(err) {
		pr_warn("qvio_user_job_buf_cleanup() failed, err=%d", err);
	}
	err = 0;

	return;
}

static int __buf_prepare(struct vb2_buffer *buffer) {
	int err;
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	int plane_size;

	switch(self->current_format.type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		switch(self->current_format.fmt.pix.pixelformat) {
		case V4L2_PIX_FMT_NV12:
			plane_size = vb2_plane_size(buffer, 0);
			if(plane_size < ZZ_ALIGN(self->current_format.fmt.pix.width, HALIGN) * self->current_format.fmt.pix.height * 3 / 2) {
				pr_err("unexpected value, plane_size=%d\n", plane_size);

				err = -EINVAL;
				goto err0;
			}
			vb2_set_plane_payload(buffer, 0, plane_size);
			break;

		default:
			pr_err("unexpected value, self->current_format.fmt.pix.pixelformat=0x%X\n", (int)self->current_format.fmt.pix.pixelformat);

			err = -EINVAL;
			goto err0;
			break;
		}
		break;

	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		switch(self->current_format.fmt.pix_mp.pixelformat) {
		case V4L2_PIX_FMT_NV12:
			plane_size = vb2_plane_size(buffer, 0);
			if(plane_size < ZZ_ALIGN(self->current_format.fmt.pix_mp.width, HALIGN) * self->current_format.fmt.pix_mp.height) {
				pr_err("unexpected value, plane_size=%d\n", plane_size);

				err = -EINVAL;
				goto err0;
			}
			vb2_set_plane_payload(buffer, 0, plane_size);

			plane_size = vb2_plane_size(buffer, 1);
			if(plane_size < ZZ_ALIGN(self->current_format.fmt.pix_mp.width, HALIGN) * self->current_format.fmt.pix_mp.height / 2) {
				pr_err("unexpected value, plane_size=%d\n", plane_size);

				err = -EINVAL;
				goto err0;
			}
			vb2_set_plane_payload(buffer, 1, plane_size);
			break;

		default:
			pr_err("unexpected value, self->current_format.fmt.pix_mp.pixelformat=0x%X\n", (int)self->current_format.fmt.pix_mp.pixelformat);

			err = -EINVAL;
			goto err0;
			break;
		}
		break;

	default:
		pr_err("unexpected value, self->current_format.type=%d\n", (int)self->current_format.type);

		err = -EINVAL;
		goto err0;
		break;
	}

	if(vbuf->field == V4L2_FIELD_ANY)
		vbuf->field = V4L2_FIELD_NONE;

	err = 0;

	return err;

err0:
	return err;
}

static void __buf_queue(struct vb2_buffer *buffer) {
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	struct qvio_queue_buffer* buf = container_of(vbuf, struct qvio_queue_buffer, vb);

#if 0 // DEBUG
	pr_info("param: %p %p %d %p\n", self, vbuf, vbuf->vb2_buf.index, buf);
#endif

	if (!mutex_lock_interruptible(&self->buffers_mutex)) {
		list_add_tail(&buf->list, &self->buffers);
		mutex_unlock(&self->buffers_mutex);
	}
}

static int __start_streaming(struct vb2_queue *queue, unsigned int count) {
	int err;
	struct qvio_queue* self = container_of(queue, struct qvio_queue, queue);
	struct qvio_device* device = container_of(self, struct qvio_device, queue);

	pr_info("\n");

	err = qvio_user_job_start_streaming(&device->user_job_ctrl);
	if(err) {
		pr_warn("qvio_user_job_start_streaming() failed, err=%d", err);
	}
	err = 0;

	self->sequence = 0;
	err = 0;

	return err;
}

static void __stop_streaming(struct vb2_queue *queue) {
	int err;
	struct qvio_queue* self = container_of(queue, struct qvio_queue, queue);
	struct qvio_device* device = container_of(self, struct qvio_device, queue);

	pr_info("\n");

	err = qvio_user_job_stop_streaming(&device->user_job_ctrl);
	if(err) {
		pr_warn("qvio_user_job_stop_streaming() failed, err=%d", err);
	}
	err = 0;

	if (!mutex_lock_interruptible(&self->buffers_mutex)) {
		struct qvio_queue_buffer* buf;
		struct qvio_queue_buffer* node;

		list_for_each_entry_safe(buf, node, &self->buffers, list) {
#if 0 // DEBUG
			pr_info("vb2_buffer_done: %p %d\n", buf, buf->vb.vb2_buf.index);
#endif

			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
			list_del(&buf->list);
		}

		mutex_unlock(&self->buffers_mutex);
	}
}

static const struct vb2_ops qvio_vb2_ops = {
	.queue_setup     = __queue_setup,
	.buf_init        = __buf_init,
	.buf_cleanup     = __buf_cleanup,
	.buf_prepare     = __buf_prepare,
	.buf_queue       = __buf_queue,
	.start_streaming = __start_streaming,
	.stop_streaming  = __stop_streaming,
	.wait_prepare    = vb2_ops_wait_prepare,
	.wait_finish     = vb2_ops_wait_finish,
};

int qvio_queue_start(struct qvio_queue* self, enum v4l2_buf_type type) {
	pr_info("\n");

	self->queue.type = type;
	if(type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
		type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		self->queue.io_modes = VB2_READ;
	else
		self->queue.io_modes = VB2_WRITE;
	self->queue.io_modes |= VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	self->queue.drv_priv = self;
	self->queue.lock = &self->queue_mutex;
	self->queue.buf_struct_size = sizeof(struct qvio_queue_buffer);
	self->queue.mem_ops = &vb2_vmalloc_memops;
	self->queue.ops = &qvio_vb2_ops;
	self->queue.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	self->queue.min_buffers_needed = 2;

	return 0;
}

void qvio_queue_stop(struct qvio_queue* self) {
	pr_info("\n");
}

struct vb2_queue* qvio_queue_get_vb2_queue(struct qvio_queue* self) {
	return &self->queue;
}

int qvio_queue_s_fmt(struct qvio_queue* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	memcpy(&self->current_format, format, sizeof(struct v4l2_format));
	err = 0;

	return err;
}

int qvio_queue_g_fmt(struct qvio_queue* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	memcpy(format, &self->current_format, sizeof(struct v4l2_format));
	err = 0;

	return err;
}

int qvio_queue_try_buf_done(struct qvio_queue* self) {
	struct qvio_device* device = container_of(self, struct qvio_device, queue);
	struct qvio_queue_buffer* buf;
	int err;

	err = mutex_lock_interruptible(&self->buffers_mutex);
	if (err) {
		pr_err("mutex_lock_interruptible() failed, err=%d\n", err);

		goto err0;
	}

	if (list_empty(&self->buffers)) {
		mutex_unlock(&self->buffers_mutex);
		pr_err("unexpected, list_empty()\n");

		goto err0;
	}

	buf = list_entry(self->buffers.next, struct qvio_queue_buffer, list);
	list_del(&buf->list);
	buf->vb.vb2_buf.timestamp = ktime_get_ns();
	buf->vb.field = V4L2_FIELD_NONE;
	buf->vb.sequence = self->sequence++;
	mutex_unlock(&self->buffers_mutex);

#if 0 // DEBUG
	pr_info("vb2_buffer_done: %p %d\n", buf, buf->vb.vb2_buf.index);
#endif

	err = qvio_user_job_buf_done(&device->user_job_ctrl, &buf->vb.vb2_buf);
	if(err) {
		pr_warn("qvio_user_job_buf_done() failed, err=%d", err);
	}
	err = 0;

	vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_DONE);

	return err;

err0:
	return err;
}