#ifndef __QVIO_DEVICE_H__
#define __QVIO_DEVICE_H__

#include "queue.h"
#include "user_job.h"
#include "uapi/qvio.h"

#include <media/v4l2-device.h>
#include <linux/videodev2.h>

#define QVIO_DRIVER_NAME "qvio"

#define HALIGN 0x0010
#define ZZ_ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

struct qvio_device {
	struct kref ref;

	// v4l2
	struct qvio_queue queue;
	struct mutex device_mutex;
	struct v4l2_device v4l2_dev;
	struct video_device *vdev;
	int vfl_dir;
	u32 device_caps;
	enum v4l2_buf_type buffer_type;
	struct v4l2_format current_format;
	unsigned int current_inout;
	struct v4l2_streamparm current_parm;

	// cdev
	dev_t cdevno;
	struct cdev cdev;

	// user job
	struct qvio_user_job_dev user_job;
};

int qvio_device_register(void);
void qvio_device_unregister(void);

struct qvio_device* qvio_device_new(void);
struct qvio_device* qvio_device_get(struct qvio_device* self);
void qvio_device_put(struct qvio_device* self);

int qvio_device_start(struct qvio_device* self);
void qvio_device_stop(struct qvio_device* self);

int qvio_device_enum_fmt(struct qvio_device* self, struct v4l2_fmtdesc *format);
int qvio_device_g_fmt(struct qvio_device* self, struct v4l2_format *format);
int qvio_device_s_fmt(struct qvio_device* self, struct v4l2_format *format);
int qvio_device_try_fmt(struct qvio_device* self, struct v4l2_format *format);
int qvio_device_enum_input(struct qvio_device* self, struct v4l2_input *input);
int qvio_device_g_input(struct qvio_device* self, unsigned int *input);
int qvio_device_s_input(struct qvio_device* self, unsigned int input);
int qvio_device_enum_output(struct qvio_device* self, struct v4l2_output *output);
int qvio_device_g_output(struct qvio_device* self, unsigned int *output);
int qvio_device_s_output(struct qvio_device* self, unsigned int output);
int qvio_device_g_parm(struct qvio_device* self, struct v4l2_streamparm *param);
int qvio_device_s_parm(struct qvio_device* self, struct v4l2_streamparm *param);
int qvio_device_enum_framesizes(struct qvio_device* self, struct v4l2_frmsizeenum *frame_sizes);
int qvio_device_enum_frameintervals(struct qvio_device* self, struct v4l2_frmivalenum *frame_intervals);

// proprietary v4l2 ioctl
long qvio_device_buf_done(struct qvio_device* self);

#endif // __QVIO_DEVICE_H__