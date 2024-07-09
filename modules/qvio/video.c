#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "video.h"
#include "device.h"
#include "ioctl.h"
#include "user_job.h"

#include <linux/kernel.h>
#include <media/videobuf2-v4l2.h>

struct qvio_video* qvio_video_new(void) {
	struct qvio_video* self = kzalloc(sizeof(struct qvio_video), GFP_KERNEL);

	kref_init(&self->ref);
	qvio_queue_init(&self->queue);
	mutex_init(&self->device_mutex);
	self->vfl_dir = VFL_DIR_RX;
	self->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	self->buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	self->halign = 0x40;
	self->valign = 1;
	qvio_user_job_init(&self->user_job_ctrl);

	return self;
}

struct qvio_video* qvio_video_get(struct qvio_video* self) {
	if (self)
		kref_get(&self->ref);

	return self;
}

static void qvio_video_free(struct kref *ref)
{
	struct qvio_video* self = container_of(ref, struct qvio_video, ref);

	pr_info("\n");

	qvio_user_job_uninit(&self->user_job_ctrl);
	kfree(self);
}

void qvio_video_put(struct qvio_video* self) {
	if (self)
		kref_put(&self->ref, qvio_video_free);
}

static const struct v4l2_file_operations qvio_video_fops = {
	.owner          = THIS_MODULE    ,
	.open           = v4l2_fh_open   ,
	.release        = vb2_fop_release,
	.unlocked_ioctl = video_ioctl2   ,
	.read           = vb2_fop_read   ,
	.write          = vb2_fop_write  ,
	.mmap           = vb2_fop_mmap   ,
	.poll           = vb2_fop_poll   ,
};

int qvio_video_start(struct qvio_video* self) {
	int err;
	struct vb2_queue* vb2_queue;

	pr_info("self=%p\n", self);

	err = v4l2_device_register(NULL, &self->v4l2_dev);
	if(err) {
		pr_err("v4l2_device_register() failed, err=%d\n", err);
		goto err0;
	}

	self->queue.halign = self->halign;
	self->queue.valign = self->valign;

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
			ALIGN(self->current_format.fmt.pix.width, self->halign);
		self->current_format.fmt.pix.sizeimage =
			self->current_format.fmt.pix.bytesperline * ALIGN(self->current_format.fmt.pix.height, self->valign) * 3 / 2;

#if 0 // DEBUG
		pr_info("bytesperline=%d sizeimage=%d\n",
			(int)self->current_format.fmt.pix.bytesperline,
			(int)self->current_format.fmt.pix.sizeimage);
#endif
		break;

	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		self->current_format.fmt.pix_mp.width = 1920;
		self->current_format.fmt.pix_mp.height = 1080;
		self->current_format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
		self->current_format.fmt.pix_mp.num_planes = 2;
		self->current_format.fmt.pix_mp.plane_fmt[0].bytesperline =
			ALIGN(self->current_format.fmt.pix_mp.width, self->halign);
		self->current_format.fmt.pix_mp.plane_fmt[0].sizeimage =
			self->current_format.fmt.pix_mp.plane_fmt[0].bytesperline * ALIGN(self->current_format.fmt.pix_mp.height, self->valign);
		self->current_format.fmt.pix_mp.plane_fmt[1].bytesperline =
			ALIGN(self->current_format.fmt.pix_mp.width, self->halign);
		self->current_format.fmt.pix_mp.plane_fmt[1].sizeimage =
			self->current_format.fmt.pix_mp.plane_fmt[1].bytesperline * ALIGN(self->current_format.fmt.pix_mp.height, self->valign) / 2;

#if 0 // DEBUG
		pr_info("bytesperline=%d/%d sizeimage=%d/%d\n",
			(int)self->current_format.fmt.pix_mp.plane_fmt[0].bytesperline,
			(int)self->current_format.fmt.pix_mp.plane_fmt[1].bytesperline,
			(int)self->current_format.fmt.pix_mp.plane_fmt[0].sizeimage,
			(int)self->current_format.fmt.pix_mp.plane_fmt[1].sizeimage);
#endif
		break;

	default:
		pr_err("unexpected value, self->buffer_type=%d\n", (int)self->buffer_type);
		goto err3;
		break;
	}

	self->current_parm.type = self->buffer_type;
	self->current_parm.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	self->current_parm.parm.capture.capturemode = 0;
	self->current_parm.parm.capture.timeperframe.numerator = 60;
	self->current_parm.parm.capture.timeperframe.denominator = 1;

	snprintf(self->vdev->name, 32, "%s", self->v4l2_dev.name);
	self->vdev->v4l2_dev = &self->v4l2_dev;
	self->vdev->vfl_dir = self->vfl_dir;
	self->vdev->fops = &qvio_video_fops;
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

void qvio_video_stop(struct qvio_video* self) {
	pr_info("\n");

	video_unregister_device(self->vdev);
	video_device_release(self->vdev);
	qvio_queue_stop(&self->queue);
	v4l2_device_unregister(&self->v4l2_dev);
}

int qvio_video_s_fmt(struct qvio_video* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	err = qvio_video_try_fmt(self, format);
	if(err) {
		pr_err("qvio_video_try_fmt() failed, err=%d", err);
		goto err0;
	}

	switch(format->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		switch(format->fmt.pix.pixelformat) {
		case V4L2_PIX_FMT_YUYV:
			format->fmt.pix.bytesperline = ALIGN(format->fmt.pix.width * 2, self->halign);
			format->fmt.pix.sizeimage = format->fmt.pix.bytesperline * ALIGN(format->fmt.pix.height, self->valign);
			break;

		case V4L2_PIX_FMT_NV12:
			format->fmt.pix.bytesperline = ALIGN(format->fmt.pix.width, self->halign);
			format->fmt.pix.sizeimage = format->fmt.pix.bytesperline * ALIGN(format->fmt.pix.height, self->valign) * 3 / 2;
			break;

		default:
			pr_err("invalid value, format->fmt.pix.pixelformat=%d", (int)format->fmt.pix.pixelformat);
			err = -EINVAL;
			goto err0;
			break;
		}

#if 0 // DEBUG
		pr_info("%dx%d bytesperline=%d sizeimage=%d\n",
			format->fmt.pix.width, format->fmt.pix.height,
			(int)format->fmt.pix.bytesperline,
			(int)format->fmt.pix.sizeimage);
#endif

		break;

	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		switch(format->fmt.pix_mp.pixelformat) {
		case V4L2_PIX_FMT_YUYV:
			format->fmt.pix_mp.num_planes = 1;
			format->fmt.pix_mp.plane_fmt[0].bytesperline = ALIGN(format->fmt.pix_mp.width * 2, self->halign);
			format->fmt.pix_mp.plane_fmt[0].sizeimage =
				format->fmt.pix_mp.plane_fmt[0].bytesperline * ALIGN(format->fmt.pix_mp.height, self->valign);
			break;

		case V4L2_PIX_FMT_NV12:
			format->fmt.pix_mp.num_planes = 2;
			format->fmt.pix_mp.plane_fmt[0].bytesperline = ALIGN(format->fmt.pix_mp.width, self->halign);
			format->fmt.pix_mp.plane_fmt[0].sizeimage =
				format->fmt.pix_mp.plane_fmt[0].bytesperline * ALIGN(format->fmt.pix_mp.height, self->valign);
			format->fmt.pix_mp.plane_fmt[1].bytesperline = format->fmt.pix_mp.plane_fmt[0].bytesperline;
			format->fmt.pix_mp.plane_fmt[1].sizeimage =
				format->fmt.pix_mp.plane_fmt[1].bytesperline * ALIGN(format->fmt.pix_mp.height, self->valign) / 2;
			break;

		default:
			pr_err("invalid value, format->fmt.pix_mp.pixelformat=%d", (int)format->fmt.pix_mp.pixelformat);
			err = -EINVAL;
			goto err0;
			break;
		}

#if 0 // DEBUG
		pr_info("%dx%d bytesperline={%d,%d,%d,%d} sizeimage={%d,%d,%d,%d}\n",
			format->fmt.pix_mp.width, format->fmt.pix_mp.height,
			(int)format->fmt.pix_mp.plane_fmt[0].bytesperline,
			(int)format->fmt.pix_mp.plane_fmt[1].bytesperline,
			(int)format->fmt.pix_mp.plane_fmt[2].bytesperline,
			(int)format->fmt.pix_mp.plane_fmt[3].bytesperline,
			(int)format->fmt.pix_mp.plane_fmt[0].sizeimage,
			(int)format->fmt.pix_mp.plane_fmt[1].sizeimage,
			(int)format->fmt.pix_mp.plane_fmt[2].sizeimage,
			(int)format->fmt.pix_mp.plane_fmt[3].sizeimage);
#endif

		break;

	default:
		pr_err("unexpected value, format->type=%d\n", (int)format->type);
		err = -EINVAL;
		goto err0;
		break;
	}

	memcpy(&self->current_format, format, sizeof(struct v4l2_format));

	err = qvio_queue_s_fmt(&self->queue, format);
	if(err) {
		pr_err("qvio_queue_s_fmt() failed, err=%d", err);
		goto err0;
	}

	err = qvio_user_job_s_fmt(&self->user_job_ctrl, format);
	if(err) {
		pr_warn("qvio_user_job_s_fmt() failed, err=%d", err);
	}
	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_enum_fmt(struct qvio_video* self, struct v4l2_fmtdesc *format) {
	int err;

	pr_info("\n");

	if(format->type != self->buffer_type) {
		pr_err("unexpected value, %d != %d\n", (int)format->type, (int)self->buffer_type);
		err = -EINVAL;

		goto err0;
	}

	switch(format->index) {
	case 0:
		format->flags = 0;
		snprintf((char *) format->description, 32, "YUYV");
		format->pixelformat = V4L2_PIX_FMT_YUYV;
		format->mbus_code = 0;
		break;

	case 1:
		format->flags = 0;
		snprintf((char *) format->description, 32, "NV12");
		format->pixelformat = V4L2_PIX_FMT_NV12;
		format->mbus_code = 0;
		break;

	default:
		pr_err("unexpected value, format->index=%d\n", (int)format->index);
		err = -EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_g_fmt(struct qvio_video* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	memcpy(format, &self->current_format, sizeof(struct v4l2_format));
	err = 0;

	return err;
}

int qvio_video_try_fmt(struct qvio_video* self, struct v4l2_format *format) {
	int err;

	pr_info("\n");

	if(format->type != self->buffer_type) {
		pr_err("unexpected value, %d != %d\n", (int)format->type, (int)self->buffer_type);
		err = -EINVAL;

		goto err0;
	}

	switch(self->buffer_type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		switch(format->fmt.pix.pixelformat) {
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_NV12:
			break;

		default:
			pr_err("unexpected value, format->fmt.pix.pixelformat=0x%X\n", (int)format->fmt.pix.pixelformat);
			err = -EINVAL;

			goto err0;
			break;
		}
		break;

	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		switch(format->fmt.pix_mp.pixelformat) {
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_NV12:
			break;

		default:
			pr_err("unexpected value, format->fmt.pix_mp.pixelformat=0x%X\n", (int)format->fmt.pix_mp.pixelformat);
			err = -EINVAL;

			goto err0;
			break;
		}
		break;

	default:
		pr_err("unexpected value, self->buffer_type=%d\n", (int)self->buffer_type);
		err = -EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_enum_input(struct qvio_video* self, struct v4l2_input *input) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_RX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = -EINVAL;

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
		err = -EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_g_input(struct qvio_video* self, unsigned int *input) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_RX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = -EINVAL;

		goto err0;
	}

	*input = self->current_inout;
	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_s_input(struct qvio_video* self, unsigned int input) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_RX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = -EINVAL;

		goto err0;
	}

	switch(input) {
	case 0:
		self->current_inout = input;
		break;

	default:
		pr_err("unexpected value, input=%d\n", (int)input);
		err = -EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_enum_output(struct qvio_video* self, struct v4l2_output *output) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_TX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = -EINVAL;

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
		err = -EINVAL;

		goto err0;
		break;
	}

	return err;

err0:
	return err;
}

int qvio_video_g_output(struct qvio_video* self, unsigned int *output) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_TX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = -EINVAL;

		goto err0;
	}

	*output = self->current_inout;
	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_s_output(struct qvio_video* self, unsigned int output) {
	int err;

	pr_info("\n");

	if(self->vfl_dir != VFL_DIR_TX) {
		pr_err("unexpected value, self->vfl_dir=%d\n", (int)self->vfl_dir);
		err = -EINVAL;

		goto err0;
	}

	switch(output) {
	case 0:
		self->current_inout = output;
		break;

	default:
		pr_err("unexpected value, output=%d\n", (int)output);
		err = -EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_g_parm(struct qvio_video* self, struct v4l2_streamparm *param) {
	int err;

	pr_info("\n");

	if(param->type != self->buffer_type) {
		pr_err("unexpected value, %d != %d\n", (int)param->type, (int)self->buffer_type);
		err = -EINVAL;

		goto err0;
	}

	*param = self->current_parm;
	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_s_parm(struct qvio_video* self, struct v4l2_streamparm *param) {
	int err;

	pr_info("\n");

	if(param->type != self->buffer_type) {
		pr_err("unexpected value, %d != %d\n", (int)param->type, (int)self->buffer_type);
		err = -EINVAL;

		goto err0;
	}

	self->current_parm = *param;
	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_enum_framesizes(struct qvio_video* self, struct v4l2_frmsizeenum *frame_sizes) {
	int err;

	pr_info("\n");

	switch(frame_sizes->index) {
	case 0:
		switch(frame_sizes->pixel_format) {
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_NV12:
			frame_sizes->type = V4L2_FRMSIZE_TYPE_STEPWISE;
			frame_sizes->stepwise.min_width = 64;
			frame_sizes->stepwise.max_width = 4096;
			frame_sizes->stepwise.step_width = 8;
			frame_sizes->stepwise.min_height = 64;
			frame_sizes->stepwise.max_height = 4096;
			frame_sizes->stepwise.step_height = 8;
			break;

		default:
			pr_err("unexpected value, frame_sizes->pixel_format=0x%X\n", (int)frame_sizes->pixel_format);
			err = -EINVAL;

			goto err0;
			break;
		}
		break;

	default:
		pr_err("unexpected value, frame_sizes->index=%d\n", (int)frame_sizes->index);
		err = -EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

int qvio_video_enum_frameintervals(struct qvio_video* self, struct v4l2_frmivalenum *frame_intervals) {
	int err;

	pr_info("\n");

	switch(frame_intervals->index) {
	case 0:
		switch(frame_intervals->pixel_format) {
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_NV12:
			frame_intervals->type = V4L2_FRMIVAL_TYPE_DISCRETE;
			frame_intervals->discrete.numerator = 1;
			frame_intervals->discrete.denominator = 60;
			break;

		default:
			pr_err("unexpected value, frame_intervals->pixel_format=0x%X\n", (int)frame_intervals->pixel_format);
			err = -EINVAL;

			goto err0;
			break;
		}
		break;

	default:
		pr_err("unexpected value, frame_intervals->index=%d\n", (int)frame_intervals->index);
		err = -EINVAL;

		goto err0;
		break;
	}

	err = 0;

	return err;

err0:
	return err;
}

long qvio_video_buf_done(struct qvio_video* self) {
	long ret;
	int err;

#if 0 // DEBUG
	pr_info("\n");
#endif

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
