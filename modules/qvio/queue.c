#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "queue.h"

#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#define ZZ_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

struct qvio_queue {
	struct kref ref;
	struct vb2_queue queue;
	struct mutex queue_mutex;
	struct list_head buffers;
	struct mutex buffers_mutex;
	struct v4l2_format current_format;
};

struct qvio_queue_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
};

static void qvio_queue_free(struct kref *ref)
{
	struct qvio_queue* self = container_of(ref, struct qvio_queue, ref);
	kfree(self);
}

struct qvio_queue* qvio_queue_new(void) {
	struct qvio_queue* self = kzalloc(sizeof(struct qvio_queue), GFP_KERNEL);

	kref_init(&self->ref);
	mutex_init(&self->queue_mutex);
	INIT_LIST_HEAD(&self->buffers);
	mutex_init(&self->buffers_mutex);

	return self;
}

struct qvio_queue* qvio_queue_get(struct qvio_queue* self) {
	if (self)
		kref_get(&self->ref);

	return self;
}

void qvio_queue_put(struct qvio_queue* self) {
	if (self)
		kref_put(&self->ref, qvio_queue_free);
}

int qvio_queue_setup(struct vb2_queue *queue,
	unsigned int *num_buffers,
	unsigned int *num_planes,
	unsigned int sizes[],
	struct device *alloc_devs[]) {
	int err = 0;
	struct qvio_queue* self = container_of(queue, struct qvio_queue, queue);

	pr_info("+param %d %d\n", *num_buffers, *num_planes);

	if(*num_buffers < 1)
		*num_buffers = 1;

	if(self->current_format.fmt.pix_mp.pixelformat == V4L2_PIX_FMT_NV12) {
		*num_planes = 2;
		sizes[0] = ZZ_ALIGN(self->current_format.fmt.pix_mp.width, 0x1000) * self->current_format.fmt.pix_mp.height;
		sizes[1] = ZZ_ALIGN(self->current_format.fmt.pix_mp.width, 0x1000) * self->current_format.fmt.pix_mp.height / 2;
	} else {
		pr_err("invalid value, self->current_format.fmt.pix_mp.pixelformat=%d", (int)self->current_format.fmt.pix_mp.pixelformat);
		err = -EINVAL;
		goto err0;
	}

	pr_info("-param %d %d [%d %d]\n", *num_buffers, *num_planes, sizes[0], sizes[1]);

	return err;

err0:
	return err;
}

int qvio_buffer_prepare(struct vb2_buffer *buffer) {
	int err;
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	struct qvio_queue_buffer* buf = container_of(vbuf, struct qvio_queue_buffer, vb);

	pr_info("param: %p %p %d %p\n", self, vbuf, vbuf->vb2_buf.index, buf);
	err = 0;

	return err;
}

void qvio_buffer_queue(struct vb2_buffer *buffer) {
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	struct qvio_queue_buffer* buf = container_of(vbuf, struct qvio_queue_buffer, vb);

	pr_info("param: %p %p %d %p\n", self, vbuf, vbuf->vb2_buf.index, buf);

	if (!mutex_lock_interruptible(&self->buffers_mutex)) {
		list_add_tail(&buf->list, &self->buffers);
		mutex_unlock(&self->buffers_mutex);
	}
}

int qvio_start_streaming(struct vb2_queue *queue, unsigned int count) {
	int err;

	pr_info("\n");
	err = 0;

	return err;
}

void qvio_stop_streaming(struct vb2_queue *queue) {
	struct qvio_queue* self = container_of(queue, struct qvio_queue, queue);

	pr_info("\n");

	if (!mutex_lock_interruptible(&self->buffers_mutex)) {
		struct qvio_queue_buffer* buf;
		struct qvio_queue_buffer* node;

		list_for_each_entry_safe(buf, node, &self->buffers, list) {
			pr_info("vb2_buffer_done: %p %d\n", buf, buf->vb.vb2_buf.index);

			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
			list_del(&buf->list);
		}

		mutex_unlock(&self->buffers_mutex);
	}

}

int qvio_buffer_init(struct vb2_buffer *buffer) {
	int err;
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	struct qvio_queue_buffer* buf = container_of(vbuf, struct qvio_queue_buffer, vb);

	pr_info("param: %p %p %d %p\n", self, vbuf, vbuf->vb2_buf.index, buf);
	err = 0;

	return err;
}

void qvio_buffer_cleanup(struct vb2_buffer *buffer) {
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);
	struct qvio_queue_buffer* buf = container_of(vbuf, struct qvio_queue_buffer, vb);

	pr_info("param: %p %p %d %p\n", self, vbuf, vbuf->vb2_buf.index, buf);
}

static const struct vb2_ops qvio_vb2_ops = {
	.queue_setup     = qvio_queue_setup,
	.buf_prepare     = qvio_buffer_prepare,
	.buf_queue       = qvio_buffer_queue,
	.start_streaming = qvio_start_streaming,
	.stop_streaming  = qvio_stop_streaming,
	.wait_prepare    = vb2_ops_wait_prepare,
	.wait_finish     = vb2_ops_wait_finish,
	.buf_init        = qvio_buffer_init,
	.buf_cleanup     = qvio_buffer_cleanup,
};

int qvio_queue_start(struct qvio_queue* self, enum v4l2_buf_type type) {
	pr_info("\n");

	self->queue.type = type;
	if(type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
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
