#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "device.h"
#include "ioctl.h"

#include <linux/platform_device.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>

struct qvio_device {
	struct kref ref;
	struct qvio_queue* queue;
	struct mutex device_mutex;
	struct v4l2_device v4l2_dev;
	struct video_device *vdev;
	int videonr;
    enum v4l2_buf_type buffer_type;
 	struct v4l2_format current_format;
};

static struct qvio_device* g_dev;

static int qvio_probe(struct platform_device *pdev) {
	int err = 0;

	pr_info("\n");

	g_dev = qvio_device_new();
	if(! g_dev) {
		pr_err("qvio_device_new() failed\n");
		err = -ENOMEM;
		goto err0;
	}

	err = qvio_device_start(g_dev);
	if(err) {
		pr_err("qvio_device_start() failed, err=%d\n", err);
		goto err1;
	}

	return err;

err1:
	qvio_device_put(g_dev);
err0:
	return err;
}

static int qvio_remove(struct platform_device *pdev) {
	int err = 0;

	pr_info("\n");

	qvio_device_stop(g_dev);
	qvio_device_put(g_dev);

	return err;
}

static struct platform_driver qvio_driver = {
	.driver = {
		.name = QVIO_DRIVER_NAME
	},
	.probe  = qvio_probe,
	.remove = qvio_remove,
};

int qvio_device_register(void) {
	int err;

	err = platform_driver_register(&qvio_driver);
	if(err) {
		pr_err("platform_driver_register() failed\n");
		goto err0;
	}

	return err;

err0:
	return err;
}

void qvio_device_unregister(void) {
	platform_driver_unregister(&qvio_driver);
}

static void qvio_device_free(struct kref *ref)
{
	struct qvio_device* self = container_of(ref, struct qvio_device, ref);

	pr_info("\n");

	qvio_queue_put(self->queue);
	kfree(self);
}

struct qvio_device* qvio_device_new(void) {
	struct qvio_device* self = kzalloc(sizeof(struct qvio_device), GFP_KERNEL);

	kref_init(&self->ref);
	self->queue = qvio_queue_new();
	mutex_init(&self->device_mutex);
    snprintf(self->v4l2_dev.name, V4L2_DEVICE_NAME_SIZE, "qvid-%u", 0);
	self->videonr = -1;
	self->buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

	return self;
}

struct qvio_device* qvio_device_get(struct qvio_device* self) {
	if (self)
		kref_get(&self->ref);

	return self;
}

void qvio_device_put(struct qvio_device* self) {
	if (self)
		kref_put(&self->ref, qvio_device_free);
}

static const struct v4l2_file_operations qvio_device_fops = {
	.owner          = THIS_MODULE    ,
	.open           = v4l2_fh_open   ,
	.release        = vb2_fop_release,
	.unlocked_ioctl = video_ioctl2   ,
	.read           = vb2_fop_read   ,
	.write          = vb2_fop_write  ,
	.mmap           = vb2_fop_mmap   ,
	.poll           = vb2_fop_poll   ,
};

int qvio_device_start(struct qvio_device* self) {
	int ret;
	struct vb2_queue* vb2_queue;

	pr_info("\n");

	ret = v4l2_device_register(NULL, &self->v4l2_dev);
	if(ret) {
		pr_err("v4l2_device_register() failed, err=%d\n", ret);
		goto err0;
	}

	ret = qvio_queue_start(self->queue, self->buffer_type);
	if(ret) {
		pr_err("qvio_queue_start() failed, err=%d\n", ret);
		goto err1;
	}

	vb2_queue = qvio_queue_get_vb2_queue(self->queue);
	ret = vb2_queue_init(vb2_queue);
	if(ret) {
		pr_err("vb2_queue_init() failed, err=%d\n", ret);
		goto err2;
	}

	self->vdev = video_device_alloc();
	if(! self->vdev) {
		pr_err("video_device_alloc() failed\n");
		goto err3;
	}

	snprintf(self->vdev->name, 32, "%s", "qvio");
	self->vdev->v4l2_dev = &self->v4l2_dev;
	self->vdev->vfl_type = VFL_TYPE_VIDEO;
	self->vdev->vfl_dir = VFL_DIR_RX;
	self->vdev->minor = -1;
	self->vdev->fops = &qvio_device_fops;
	self->vdev->ioctl_ops = qvio_ioctl_ops();
	self->vdev->tvnorms = V4L2_STD_ALL;
	self->vdev->release = video_device_release_empty;
	self->vdev->queue = vb2_queue;
	self->vdev->lock = &self->device_mutex;
	// self->vdev->ctrl_handler = akvcam_controls_handler(self->controls);
	// self->vdev->dev.groups = akvcam_attributes_groups(self->type);
	video_set_drvdata(self->vdev, self);
	self->vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE;
	ret = video_register_device(self->vdev, VFL_TYPE_VIDEO, self->videonr);
	if(ret) {
		pr_err("video_register_device() failed, err=%d\n", ret);
		goto err4;
	}

	return 0;

err5:
	video_unregister_device(self->vdev);
err4:
	video_device_release(self->vdev);
	self->vdev = NULL;
err3:
err2:
	qvio_queue_stop(self->queue);
err1:
	v4l2_device_unregister(&self->v4l2_dev);
err0:
	return ret;
}

void qvio_device_stop(struct qvio_device* self) {
	int ret;

	pr_info("\n");

	qvio_queue_stop(self->queue);

	video_unregister_device(self->vdev);
	video_device_release(self->vdev);
	self->vdev = NULL;

	v4l2_device_unregister(&self->v4l2_dev);
}

int qvio_device_s_fmt(struct qvio_device* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	memcpy(&self->current_format, format, sizeof(struct v4l2_format));
	err = qvio_queue_s_fmt(self->queue, format);
	if(err) {
		pr_err("qvio_queue_s_fmt() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_device_g_fmt(struct qvio_device* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	memcpy(format, &self->current_format, sizeof(struct v4l2_format));
	err = 0;

	return err;
}

int qvio_device_try_fmt(struct qvio_device* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	err = 0;

	return err;
}
