#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "device.h"
#include "ioctl.h"
#include "cdev.h"
#include "user_job.h"

#include <linux/platform_device.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-ioctl.h>

static struct qvio_device* g_dev_rx;
static struct qvio_device* g_dev_tx;

static int probe(struct platform_device *pdev) {
	int err = 0;

	pr_info("\n");

	g_dev_rx = qvio_device_new();
	if(! g_dev_rx) {
		pr_err("qvio_device_new() failed\n");
		err = -ENOMEM;
		goto err0;
	}

	g_dev_rx->vfl_dir = VFL_DIR_RX;
	g_dev_rx->buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	g_dev_rx->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	snprintf(g_dev_rx->v4l2_dev.name, V4L2_DEVICE_NAME_SIZE, "qvio-rx");

	err = qvio_cdev_start(g_dev_rx);
	if(err) {
		pr_err("qvio_cdev_start() failed, err=%d\n", err);
		goto err1;
	}

	err = qvio_device_start(g_dev_rx);
	if(err) {
		pr_err("qvio_device_start() failed, err=%d\n", err);
		goto err2;
	}

	g_dev_tx = qvio_device_new();
	if(! g_dev_tx) {
		pr_err("qvio_device_new() failed\n");
		err = -ENOMEM;
		goto err3;
	}

	g_dev_tx->vfl_dir = VFL_DIR_TX;
	g_dev_tx->buffer_type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	g_dev_tx->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
	snprintf(g_dev_tx->v4l2_dev.name, V4L2_DEVICE_NAME_SIZE, "qvio-tx");

	err = qvio_cdev_start(g_dev_tx);
	if(err) {
		pr_err("qvio_cdev_start() failed, err=%d\n", err);
		goto err4;
	}

	err = qvio_device_start(g_dev_tx);
	if(err) {
		pr_err("qvio_device_start() failed, err=%d\n", err);
		goto err5;
	}

	return err;

err5:
	qvio_cdev_stop(g_dev_tx);
err4:
	qvio_device_put(g_dev_tx);
err3:
	qvio_device_stop(g_dev_rx);
err2:
	qvio_cdev_stop(g_dev_rx);
err1:
	qvio_device_put(g_dev_rx);
err0:
	return err;
}

static int remove(struct platform_device *pdev) {
	int err = 0;

	pr_info("\n");

	qvio_device_stop(g_dev_tx);
	qvio_cdev_stop(g_dev_tx);
	qvio_device_put(g_dev_tx);
	qvio_device_stop(g_dev_rx);
	qvio_cdev_stop(g_dev_rx);
	qvio_device_put(g_dev_rx);

	return err;
}

static struct platform_driver qvio_driver = {
	.driver = {
		.name = QVIO_DRIVER_NAME
	},
	.probe  = probe,
	.remove = remove,
};

int qvio_device_register(void) {
	int err;

	pr_info("\n");

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
	pr_info("\n");

	platform_driver_unregister(&qvio_driver);
}

struct qvio_device* qvio_device_new(void) {
	struct qvio_device* self = kzalloc(sizeof(struct qvio_device), GFP_KERNEL);

	kref_init(&self->ref);
	qvio_queue_init(&self->queue);
	mutex_init(&self->device_mutex);
	self->vfl_dir = VFL_DIR_RX;
	self->buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	self->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	qvio_cdev_init(self);
	qvio_user_job_init(self);

	return self;
}

struct qvio_device* qvio_device_get(struct qvio_device* self) {
	if (self)
		kref_get(&self->ref);

	return self;
}

static void qvio_device_free(struct kref *ref)
{
	struct qvio_device* self = container_of(ref, struct qvio_device, ref);

	pr_info("\n");

	kfree(self);
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
	int err;
	struct vb2_queue* vb2_queue;

	pr_info("\n");

	err = v4l2_device_register(NULL, &self->v4l2_dev);
	if(err) {
		pr_err("v4l2_device_register() failed, err=%d\n", err);
		goto err0;
	}

	err = qvio_queue_start(&self->queue, self->buffer_type);
	if(err) {
		pr_err("qvio_queue_start() failed, err=%d\n", err);
		goto err1;
	}

	vb2_queue = qvio_queue_get_vb2_queue(&self->queue);
	err = vb2_queue_init(vb2_queue);
	if(err) {
		pr_err("vb2_queue_init() failed, err=%d\n", err);
		goto err2;
	}

	self->vdev = video_device_alloc();
	if(! self->vdev) {
		pr_err("video_device_alloc() failed\n");
		goto err2;
	}

	pr_info("param: %s %d\n", self->v4l2_dev.name, self->vfl_dir);

	self->current_format.type = self->buffer_type;

	switch(self->buffer_type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		self->current_format.fmt.pix.width = 1920;
		self->current_format.fmt.pix.height = 1080;
		self->current_format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
		self->current_format.fmt.pix.bytesperline =
			ZZ_ALIGN(self->current_format.fmt.pix.width, HALIGN);
		self->current_format.fmt.pix.sizeimage =
			self->current_format.fmt.pix.bytesperline * self->current_format.fmt.pix.height * 3 / 2;

		pr_info("bytesperline=%d sizeimage=%d\n",
			(int)self->current_format.fmt.pix.bytesperline,
			(int)self->current_format.fmt.pix.sizeimage);
		break;

	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		self->current_format.fmt.pix_mp.width = 1920;
		self->current_format.fmt.pix_mp.height = 1080;
		self->current_format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
		self->current_format.fmt.pix_mp.num_planes = 2;
		self->current_format.fmt.pix_mp.plane_fmt[0].bytesperline =
			ZZ_ALIGN(self->current_format.fmt.pix_mp.width, HALIGN);
		self->current_format.fmt.pix_mp.plane_fmt[0].sizeimage =
			self->current_format.fmt.pix_mp.plane_fmt[0].bytesperline * self->current_format.fmt.pix_mp.height;
		self->current_format.fmt.pix_mp.plane_fmt[1].bytesperline =
			ZZ_ALIGN(self->current_format.fmt.pix_mp.width, HALIGN);
		self->current_format.fmt.pix_mp.plane_fmt[1].sizeimage =
			self->current_format.fmt.pix_mp.plane_fmt[1].bytesperline * self->current_format.fmt.pix_mp.height / 2;

		pr_info("bytesperline=%d/%d sizeimage=%d/%d\n",
			(int)self->current_format.fmt.pix_mp.plane_fmt[0].bytesperline,
			(int)self->current_format.fmt.pix_mp.plane_fmt[1].bytesperline,
			(int)self->current_format.fmt.pix_mp.plane_fmt[0].sizeimage,
			(int)self->current_format.fmt.pix_mp.plane_fmt[1].sizeimage);
		break;

	default:
		pr_err("unexpected value, self->buffer_type=%d\n", (int)self->buffer_type);
		goto err3;
		break;
	}

	self->current_parm.type = self->buffer_type;

	snprintf(self->vdev->name, 32, "%s", self->v4l2_dev.name);
	self->vdev->v4l2_dev = &self->v4l2_dev;
	self->vdev->vfl_dir = self->vfl_dir;
	self->vdev->fops = &qvio_device_fops;
	self->vdev->ioctl_ops = qvio_ioctl_ops();
	self->vdev->tvnorms = V4L2_STD_ALL;
	self->vdev->release = video_device_release_empty;
	self->vdev->queue = vb2_queue;
	self->vdev->lock = &self->device_mutex;
	video_set_drvdata(self->vdev, self);
	self->vdev->device_caps = self->device_caps;
	err = video_register_device(self->vdev, VFL_TYPE_VIDEO, -1);
	if(err) {
		pr_err("video_register_device() failed, err=%d\n", err);
		goto err3;
	}

	return 0;

err3:
	video_device_release(self->vdev);
err2:
	qvio_queue_stop(&self->queue);
err1:
	v4l2_device_unregister(&self->v4l2_dev);
err0:
	return err;
}

void qvio_device_stop(struct qvio_device* self) {
	pr_info("\n");

	video_unregister_device(self->vdev);
	video_device_release(self->vdev);
	qvio_queue_stop(&self->queue);
	v4l2_device_unregister(&self->v4l2_dev);
}

int qvio_device_s_fmt(struct qvio_device* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	err = qvio_device_try_fmt(self, format);
	if(err) {
		pr_err("qvio_device_try_fmt() failed, err=%d", err);
		goto err0;
	}

	qvio_user_job_s_fmt(self, format);

	memcpy(&self->current_format, format, sizeof(struct v4l2_format));
	err = qvio_queue_s_fmt(&self->queue, format);
	if(err) {
		pr_err("qvio_queue_s_fmt() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_device_enum_fmt(struct qvio_device* self, struct v4l2_fmtdesc *format) {
	int err;

	pr_info("\n");

	if(format->type != self->buffer_type) {
		pr_err("unexpected value, %d != %d\n", (int)format->type, (int)self->buffer_type);
		err = EINVAL;

		goto err0;
	}

	switch(format->index) {
	case 0:
		format->flags = 0;
		snprintf((char *) format->description, 32, "NV12");
		format->pixelformat = V4L2_PIX_FMT_NV12;
		format->mbus_code = 0;
		break;

	default:
		pr_err("unexpected value, format->index=%d\n", (int)format->index);
		err = EINVAL;

		goto err0;
		break;
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

	if(format->type != self->buffer_type) {
		pr_err("unexpected value, %d != %d\n", (int)format->type, (int)self->buffer_type);
		err = EINVAL;

		goto err0;
	}

	switch(self->buffer_type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		switch(format->fmt.pix.pixelformat) {
		case V4L2_PIX_FMT_NV12:
			break;

		default:
			pr_err("unexpected value, format->fmt.pix.pixelformat=0x%X\n", (int)format->fmt.pix.pixelformat);
			err = EINVAL;

			goto err0;
			break;
		}
		break;

	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		switch(format->fmt.pix_mp.pixelformat) {
		case V4L2_PIX_FMT_NV12:
			break;

		default:
			pr_err("unexpected value, format->fmt.pix_mp.pixelformat=0x%X\n", (int)format->fmt.pix_mp.pixelformat);
			err = EINVAL;

			goto err0;
			break;
		}
		break;

	default:
		pr_err("unexpected value, self->buffer_type=%d\n", (int)self->buffer_type);
		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_enum_input(struct qvio_device* self, struct v4l2_input *input) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_RX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = EINVAL;

		goto err0;
	}

	switch(input->index) {
	case 0:
		snprintf((char *) input->name, 32, "qvio-input");
		input->type = V4L2_INPUT_TYPE_CAMERA;
		input->audioset = 0;
		input->tuner = 0;
		input->std = 0;
		input->status = 0;
		input->capabilities = 0;
		break;

	default:
		pr_err("unexpected value, input->index=%d\n", (int)input->index);
		err = EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_g_input(struct qvio_device* self, unsigned int *input) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_RX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = EINVAL;

		goto err0;
	}

	*input = self->current_inout;
	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_s_input(struct qvio_device* self, unsigned int input) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_RX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = EINVAL;

		goto err0;
	}

	switch(input) {
	case 0:
		self->current_inout = input;
		break;

	default:
		pr_err("unexpected value, input=%d\n", (int)input);
		err = EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_enum_output(struct qvio_device* self, struct v4l2_output *output) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_TX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = EINVAL;

		goto err0;
	}

	switch(output->index) {
	case 0:
		snprintf((char *) output->name, 32, "qvio-output");
		output->type = V4L2_OUTPUT_TYPE_ANALOG;
		output->audioset = 0;
		output->modulator = 0;
		output->std = 0;
		output->capabilities = 0;
		break;

	default:
		pr_err("unexpected value, output->index=%d\n", (int)output->index);
		err = EINVAL;

		goto err0;
		break;
	}

	return err;

err0:
	return err;
}

int qvio_device_g_output(struct qvio_device* self, unsigned int *output) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_TX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = EINVAL;

		goto err0;
	}

	*output = self->current_inout;
	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_s_output(struct qvio_device* self, unsigned int output) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_TX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = EINVAL;

		goto err0;
	}

	switch(output) {
	case 0:
		self->current_inout = output;
		break;

	default:
		pr_err("unexpected value, output=%d\n", (int)output);
		err = EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_g_parm(struct qvio_device* self, struct v4l2_streamparm *param) {
	int err;

	pr_info("\n");

	if(param->type != self->buffer_type) {
		pr_err("unexpected value, %d != %d\n", (int)param->type, (int)self->buffer_type);
		err = EINVAL;

		goto err0;
	}

	*param = self->current_parm;
	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_s_parm(struct qvio_device* self, struct v4l2_streamparm *param) {
	int err;

	pr_info("\n");

	if(param->type != self->buffer_type) {
		pr_err("unexpected value, %d != %d\n", (int)param->type, (int)self->buffer_type);
		err = EINVAL;

		goto err0;
	}

	self->current_parm = *param;
	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_enum_framesizes(struct qvio_device* self, struct v4l2_frmsizeenum *frame_sizes) {
	int err;

	pr_info("\n");

	switch(frame_sizes->index) {
	case 0:
		switch(frame_sizes->pixel_format) {
		case V4L2_PIX_FMT_NV12:
			frame_sizes->type = V4L2_FRMSIZE_TYPE_STEPWISE;
			frame_sizes->stepwise.min_width = 64;
			frame_sizes->stepwise.max_width = 4096;
			frame_sizes->stepwise.step_width = 64;
			frame_sizes->stepwise.min_height = 64;
			frame_sizes->stepwise.max_height = 4096;
			frame_sizes->stepwise.step_height = 64;
			break;

		default:
			pr_err("unexpected value, frame_sizes->pixel_format=0x%X\n", (int)frame_sizes->pixel_format);
			err = EINVAL;

			goto err0;
			break;
		}
		break;

	default:
		pr_err("unexpected value, frame_sizes->index=%d\n", (int)frame_sizes->index);
		err = EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_device_enum_frameintervals(struct qvio_device* self, struct v4l2_frmivalenum *frame_intervals) {
	int err;

	pr_info("\n");

	err = EINVAL;

	return err;
}


long qvio_device_buf_done(struct qvio_device* self) {
	long ret;
	int err;

	pr_info("\n");

	err = qvio_queue_try_buf_done(&self->queue);
	if(err) {
		pr_err("qvio_queue_try_buf_done() failed, err=%d", err);
		ret = err;

		goto err0;
	}

	ret = 0;

	return ret;

err0:
	return ret;
}
