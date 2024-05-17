#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "queue.h"

#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

struct qvio_queue {
	struct kref ref;
	struct vb2_queue queue;
	struct mutex queue_mutex;
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
	pr_info("\n");

	return -EINVAL;
}

int qvio_buffer_prepare(struct vb2_buffer *buffer) {
	pr_info("\n");

	return -EINVAL;
}

void qvio_buffer_queue(struct vb2_buffer *buffer) {
	pr_info("\n");
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
	self->queue.io_modes = VB2_READ | VB2_MMAP;
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
