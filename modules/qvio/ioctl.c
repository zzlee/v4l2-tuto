#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "ioctl.h"
#include "device.h"
#include "cdev.h"
#include "uapi/qvio.h"

#include <linux/version.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-v4l2.h>

int qvio_ioctl_querycap(struct file *file, void *fh, struct v4l2_capability *capability) {
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	memset(capability, 0, sizeof(struct v4l2_capability));
	snprintf((char *) capability->driver, 16, "%s", "qvio driver");
	snprintf((char *) capability->card,
			 32, "qvio");
	snprintf((char *) capability->bus_info,
			 32, QVIO_CDEV_NAME "%d", MINOR(device->cdevno));
	capability->version = LINUX_VERSION_CODE;

	capability->capabilities = device->device_caps | V4L2_CAP_DEVICE_CAPS;
	capability->device_caps = device->device_caps;

	return 0;
}

int qvio_ioctl_enum_fmt(struct file *file, void *fh, struct v4l2_fmtdesc *format) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_enum_fmt(device, format);
	if(err) {
		pr_err("qvio_device_enum_fmt() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_g_fmt(struct file *file, void *fh, struct v4l2_format *format) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_g_fmt(device, format);
	if(err) {
		pr_err("qvio_device_g_fmt() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_s_fmt(struct file *file, void *fh, struct v4l2_format *format) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_s_fmt(device, format);
	if(err) {
		pr_err("qvio_device_s_fmt() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_try_fmt(struct file *file, void *fh, struct v4l2_format *format) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_try_fmt(device, format);
	if(err) {
		pr_err("qvio_device_try_fmt() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_enum_input(struct file *file, void *fh, struct v4l2_input *input) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_enum_input(device, input);
	if(err) {
		pr_err("qvio_device_enum_input() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_g_input(struct file *file, void *fh, unsigned int *input) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_g_input(device, input);
	if(err) {
		pr_err("qvio_device_g_input() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_s_input(struct file *file, void *fh, unsigned int input) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_s_input(device, input);
	if(err) {
		pr_err("qvio_device_s_input() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_enum_output(struct file *file, void *fh, struct v4l2_output *output) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_enum_output(device, output);
	if(err) {
		pr_err("qvio_device_enum_output() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_g_output(struct file *file, void *fh, unsigned int *output) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_g_output(device, output);
	if(err) {
		pr_err("qvio_device_g_output() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_s_output(struct file *file, void *fh, unsigned int output) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_s_output(device, output);
	if(err) {
		pr_err("qvio_device_s_output() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_g_parm(struct file *file, void *fh, struct v4l2_streamparm *param) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_g_parm(device, param);
	if(err) {
		pr_err("qvio_device_g_parm() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_s_parm(struct file *file, void *fh, struct v4l2_streamparm *param) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_s_parm(device, param);
	if(err) {
		pr_err("qvio_device_s_parm() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_enum_framesizes(struct file *file, void *fh, struct v4l2_frmsizeenum *frame_sizes) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_enum_framesizes(device, frame_sizes);
	if(err) {
		pr_err("qvio_device_enum_framesizes() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

int qvio_ioctl_enum_frameintervals(struct file *file, void *fh, struct v4l2_frmivalenum *frame_intervals) {
	int err;
	struct qvio_device* device = video_drvdata(file);

	pr_info("\n");

	err = qvio_device_enum_frameintervals(device, frame_intervals);
	if(err) {
		pr_err("qvio_device_enum_frameintervals() failed, err=%d", err);
		goto err0;
	}

	return err;

err0:
	return err;
}

const struct v4l2_ioctl_ops *qvio_ioctl_ops(void) {
	static const struct v4l2_ioctl_ops ops = {
		.vidioc_querycap               = qvio_ioctl_querycap,
		.vidioc_enum_fmt_vid_cap       = qvio_ioctl_enum_fmt,
		.vidioc_enum_fmt_vid_out       = qvio_ioctl_enum_fmt,
		.vidioc_g_fmt_vid_cap          = qvio_ioctl_g_fmt,
		.vidioc_g_fmt_vid_out          = qvio_ioctl_g_fmt,
		.vidioc_g_fmt_vid_cap_mplane   = qvio_ioctl_g_fmt,
		.vidioc_g_fmt_vid_out_mplane   = qvio_ioctl_g_fmt,
		.vidioc_s_fmt_vid_cap          = qvio_ioctl_s_fmt,
		.vidioc_s_fmt_vid_out          = qvio_ioctl_s_fmt,
		.vidioc_s_fmt_vid_cap_mplane   = qvio_ioctl_s_fmt,
		.vidioc_s_fmt_vid_out_mplane   = qvio_ioctl_s_fmt,
		.vidioc_try_fmt_vid_cap        = qvio_ioctl_try_fmt,
		.vidioc_try_fmt_vid_out        = qvio_ioctl_try_fmt,
		.vidioc_try_fmt_vid_cap_mplane = qvio_ioctl_try_fmt,
		.vidioc_try_fmt_vid_out_mplane = qvio_ioctl_try_fmt,
		.vidioc_reqbufs                = vb2_ioctl_reqbufs,
		.vidioc_querybuf               = vb2_ioctl_querybuf,
		.vidioc_qbuf                   = vb2_ioctl_qbuf,
		.vidioc_expbuf                 = vb2_ioctl_expbuf,
		.vidioc_dqbuf                  = vb2_ioctl_dqbuf,
		.vidioc_create_bufs            = vb2_ioctl_create_bufs,
		.vidioc_prepare_buf            = vb2_ioctl_prepare_buf,
		.vidioc_streamon               = vb2_ioctl_streamon,
		.vidioc_streamoff              = vb2_ioctl_streamoff,
		.vidioc_enum_input             = qvio_ioctl_enum_input,
		.vidioc_g_input                = qvio_ioctl_g_input,
		.vidioc_s_input                = qvio_ioctl_s_input,
		.vidioc_enum_output            = qvio_ioctl_enum_output,
		.vidioc_g_output               = qvio_ioctl_g_output,
		.vidioc_s_output               = qvio_ioctl_s_output,
		.vidioc_g_parm                 = qvio_ioctl_g_parm,
		.vidioc_s_parm                 = qvio_ioctl_s_parm,
		.vidioc_log_status             = v4l2_ctrl_log_status,
		.vidioc_enum_framesizes        = qvio_ioctl_enum_framesizes,
		.vidioc_enum_frameintervals    = qvio_ioctl_enum_frameintervals,
		.vidioc_subscribe_event        = v4l2_ctrl_subscribe_event,
		.vidioc_unsubscribe_event      = v4l2_event_unsubscribe,
	};

	return &ops;
}
