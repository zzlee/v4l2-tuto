#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "ioctl.h"
#include "video.h"
#include "uapi/qvio.h"

#include <linux/version.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-v4l2.h>
#include <linux/anon_inodes.h>

static int __ioctl_querycap(struct file *file, void *fh, struct v4l2_capability *capability) {
	struct qvio_video* self = video_drvdata(file);

	pr_info("self=%p\n", self);

	memset(capability, 0, sizeof(struct v4l2_capability));
	capability->version = LINUX_VERSION_CODE;
	snprintf(capability->driver, sizeof(capability->driver), "qvio");
	snprintf(capability->card, sizeof(capability->card), "qvio");
	strlcpy(capability->bus_info, self->bus_info, sizeof(capability->bus_info));
	capability->capabilities = self->device_caps | V4L2_CAP_DEVICE_CAPS;
	capability->device_caps = self->device_caps;

	return 0;
}

static int __ioctl_enum_fmt(struct file *file, void *fh, struct v4l2_fmtdesc *format) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_enum_fmt(self, format);
	if(err) {
		pr_err("qvio_video_enum_fmt() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_g_fmt(struct file *file, void *fh, struct v4l2_format *format) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_g_fmt(self, format);
	if(err) {
		pr_err("qvio_video_g_fmt() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_s_fmt(struct file *file, void *fh, struct v4l2_format *format) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_s_fmt(self, format);
	if(err) {
		pr_err("qvio_video_s_fmt() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_try_fmt(struct file *file, void *fh, struct v4l2_format *format) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_try_fmt(self, format);
	if(err) {
		pr_err("qvio_video_try_fmt() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_enum_input(struct file *file, void *fh, struct v4l2_input *input) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_enum_input(self, input);
	if(err) {
		pr_err("qvio_video_enum_input() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_g_input(struct file *file, void *fh, unsigned int *input) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_g_input(self, input);
	if(err) {
		pr_err("qvio_video_g_input() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_s_input(struct file *file, void *fh, unsigned int input) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_s_input(self, input);
	if(err) {
		pr_err("qvio_video_s_input() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_enum_output(struct file *file, void *fh, struct v4l2_output *output) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_enum_output(self, output);
	if(err) {
		pr_err("qvio_video_enum_output() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_g_output(struct file *file, void *fh, unsigned int *output) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_g_output(self, output);
	if(err) {
		pr_err("qvio_video_g_output() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_s_output(struct file *file, void *fh, unsigned int output) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_s_output(self, output);
	if(err) {
		pr_err("qvio_video_s_output() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_g_parm(struct file *file, void *fh, struct v4l2_streamparm *param) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_g_parm(self, param);
	if(err) {
		pr_err("qvio_video_g_parm() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_s_parm(struct file *file, void *fh, struct v4l2_streamparm *param) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_s_parm(self, param);
	if(err) {
		pr_err("qvio_video_s_parm() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_enum_framesizes(struct file *file, void *fh, struct v4l2_frmsizeenum *frame_sizes) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_enum_framesizes(self, frame_sizes);
	if(err) {
		pr_err("qvio_video_enum_framesizes() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __ioctl_enum_frameintervals(struct file *file, void *fh, struct v4l2_frmivalenum *frame_intervals) {
	int err;
	struct qvio_video* self = video_drvdata(file);

	pr_info("\n");

	err = qvio_video_enum_frameintervals(self, frame_intervals);
	if(err) {
		pr_err("qvio_video_enum_frameintervals() failed, err=%d", err);
		goto err0;
	}

	return 0;

err0:
	return err;
}

static int __anon_fd(const char* name, const struct file_operations *fops, void *priv, int flags) {
	int err;
	int fd;
	struct file* file;

	fd = get_unused_fd_flags(flags);
	if (fd < 0) {
		pr_err("get_unused_fd_flags() failed, fd=%d\n", fd);

		err = fd;
		goto err0;
	}

	file = anon_inode_getfile(name, fops, priv, flags);
	if (IS_ERR(file)) {
		err = PTR_ERR(file);
		pr_err("anon_inode_getfile() failed, err=%d\n", err);

		goto err1;
	}

	fd_install(fd, file);
	err = fd;

	return err;

err1:
	put_unused_fd(fd);
err0:
	return err;
}

static long __ioctl_default(struct file *file, void *fh, bool valid_prio, unsigned int cmd, void *arg) {
	long ret;
	struct qvio_video* self = video_drvdata(file);

#if 0
	pr_info("valid_prio=%d cmd=%d arg=%p\n", valid_prio, cmd, arg);
#endif

	switch(cmd) {
	case QVID_IOC_USER_JOB_FD: {
		int* pFd = (int*)arg;

		*pFd = __anon_fd("qvio-user-job", self->user_job_ctrl.ctrl_fops, &self->user_job_ctrl, O_RDONLY | O_CLOEXEC);
		ret = 0;

		pr_info("fd=%d\n", *pFd);
	}
		break;

	case QVID_IOC_BUF_DONE:
		ret = qvio_video_buf_done(self);
		break;

	case QVID_IOC_USER_CTRL_FD: {
		int* pFd = (int*)arg;

		if(self->user_ctrl_fops) {
			*pFd = __anon_fd("qvio-user-ctrl", self->user_ctrl_fops, self, O_RDWR | O_CLOEXEC);
			ret = 0;

			pr_info("fd=%d\n", *pFd);
		} else {
			pr_err("unexpected, self->user_ctrl_fops=%p\n", self->user_ctrl_fops);
			ret = -EINVAL;
		}
	}
		break;

	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

const struct v4l2_ioctl_ops *qvio_ioctl_ops(void) {
	static const struct v4l2_ioctl_ops ops = {
		.vidioc_querycap               = __ioctl_querycap,
		.vidioc_enum_fmt_vid_cap       = __ioctl_enum_fmt,
		.vidioc_enum_fmt_vid_out       = __ioctl_enum_fmt,
		.vidioc_g_fmt_vid_cap          = __ioctl_g_fmt,
		.vidioc_g_fmt_vid_out          = __ioctl_g_fmt,
		.vidioc_g_fmt_vid_cap_mplane   = __ioctl_g_fmt,
		.vidioc_g_fmt_vid_out_mplane   = __ioctl_g_fmt,
		.vidioc_s_fmt_vid_cap          = __ioctl_s_fmt,
		.vidioc_s_fmt_vid_out          = __ioctl_s_fmt,
		.vidioc_s_fmt_vid_cap_mplane   = __ioctl_s_fmt,
		.vidioc_s_fmt_vid_out_mplane   = __ioctl_s_fmt,
		.vidioc_try_fmt_vid_cap        = __ioctl_try_fmt,
		.vidioc_try_fmt_vid_out        = __ioctl_try_fmt,
		.vidioc_try_fmt_vid_cap_mplane = __ioctl_try_fmt,
		.vidioc_try_fmt_vid_out_mplane = __ioctl_try_fmt,
		.vidioc_reqbufs                = vb2_ioctl_reqbufs,
		.vidioc_querybuf               = vb2_ioctl_querybuf,
		.vidioc_qbuf                   = vb2_ioctl_qbuf,
		.vidioc_expbuf                 = vb2_ioctl_expbuf,
		.vidioc_dqbuf                  = vb2_ioctl_dqbuf,
		.vidioc_create_bufs            = vb2_ioctl_create_bufs,
		.vidioc_prepare_buf            = vb2_ioctl_prepare_buf,
		.vidioc_streamon               = vb2_ioctl_streamon,
		.vidioc_streamoff              = vb2_ioctl_streamoff,
		.vidioc_enum_input             = __ioctl_enum_input,
		.vidioc_g_input                = __ioctl_g_input,
		.vidioc_s_input                = __ioctl_s_input,
		.vidioc_enum_output            = __ioctl_enum_output,
		.vidioc_g_output               = __ioctl_g_output,
		.vidioc_s_output               = __ioctl_s_output,
		.vidioc_g_parm                 = __ioctl_g_parm,
		.vidioc_s_parm                 = __ioctl_s_parm,
		.vidioc_log_status             = v4l2_ctrl_log_status,
		.vidioc_enum_framesizes        = __ioctl_enum_framesizes,
		.vidioc_enum_frameintervals    = __ioctl_enum_frameintervals,
		.vidioc_subscribe_event        = v4l2_ctrl_subscribe_event,
		.vidioc_unsubscribe_event      = v4l2_event_unsubscribe,
		.vidioc_default                = __ioctl_default,
	};

	return &ops;
}
