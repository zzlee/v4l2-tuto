#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "queue.h"

#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#define ZZ_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

struct qvio_queue {
	struct kref ref;
	struct vb2_queue queue;
	struct mutex queue_mutex;
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

	if(self->current_format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		if(self->current_format.fmt.pix_mp.pixelformat == V4L2_PIX_FMT_NV12) {
			*num_planes = 2;
			sizes[0] = ZZ_ALIGN(self->current_format.fmt.pix_mp.width, 0x1000) * self->current_format.fmt.pix_mp.height;
			sizes[1] = ZZ_ALIGN(self->current_format.fmt.pix_mp.width, 0x1000) * self->current_format.fmt.pix_mp.height / 2;
		} else {
			pr_err("invalid value, self->current_format.fmt.pix_mp.pixelformat=%d", (int)self->current_format.fmt.pix_mp.pixelformat);
			err = -EINVAL;
			goto err0;
		}

	} else {
		pr_err("invalid value, self->current_format.type=%d", (int)self->current_format.type);
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

	pr_info("param: %p %p\n", self, vbuf);
	err = 0;

	return err;
}

void qvio_buffer_queue(struct vb2_buffer *buffer) {
	struct qvio_queue* self = vb2_get_drv_priv(buffer->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buffer);

	pr_info("param: %p %p\n", self, vbuf);
}

int qvio_start_streaming(struct vb2_queue *queue, unsigned int count) {
	pr_info("\n");

	return -EINVAL;
}

void qvio_stop_streaming(struct vb2_queue *queue) {
	pr_info("\n");
}

static const struct vb2_ops qvio_vb2_ops = {
	.queue_setup     = qvio_queue_setup,
	.buf_prepare     = qvio_buffer_prepare,
	.buf_queue       = qvio_buffer_queue,
	.start_streaming = qvio_start_streaming,
	.stop_streaming  = qvio_stop_streaming,
	.wait_prepare    = vb2_ops_wait_prepare,
	.wait_finish     = vb2_ops_wait_finish,
};

int qvio_queue_start(struct qvio_queue* self, enum v4l2_buf_type type) {
	pr_info("\n");

	self->queue.type = type;
	self->queue.io_modes = VB2_READ | VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
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
