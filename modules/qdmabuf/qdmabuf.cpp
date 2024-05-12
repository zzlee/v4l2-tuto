#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include "qdmabuf.h"

static struct qdmabuf_dev *alloc_dev_instance() {
	struct qdmabuf_dev* qdev;

	qdev = kzalloc(sizeof(struct qdmabuf_dev), GFP_KERNEL);
	if (!qdev) {
		pr_info("OOM, qdmabuf_dev.\n");
		return NULL;
	}
	spin_lock_init(&qdev->lock);

	dbg_init("qdev = 0x%p\n", qdev);

	return qdev;
}

void *qdmabuf_device_open(const char *mname)
{
	struct qdmabuf_dev *qdev = NULL;
	int rv = 0;

	pr_info("%s.\n", mname);

	/* allocate zeroed device book keeping structure */
	qdev = alloc_dev_instance();
	if (!qdev)
		return NULL;
	qdev->mod_name = mname;

	return qdev;
}