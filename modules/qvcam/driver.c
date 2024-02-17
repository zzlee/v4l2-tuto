#include <linux/slab.h>
#include <linux/version.h>
#include <linux/videodev2.h>

#include "log.h"
#include "utils.h"
#include "driver.h"

typedef struct
{
	char name[QVCAM_MAX_STRING_SIZE];
	char description[QVCAM_MAX_STRING_SIZE];
} qvcam_driver, *qvcam_driver_t;

static qvcam_driver_t qvcam_driver_global = NULL;

bool qvcam_driver_register(void);
void qvcam_driver_unregister(void);

int qvcam_driver_init(const char *name, const char *description)
{
	qpr_function();

	if (qvcam_driver_global)
		return -EINVAL;

	qpr_info("Initializing driver\n");
	qvcam_driver_global = kzalloc(sizeof(qvcam_driver), GFP_KERNEL);
	snprintf(qvcam_driver_global->name, QVCAM_MAX_STRING_SIZE, "%s", name);
	snprintf(qvcam_driver_global->description, QVCAM_MAX_STRING_SIZE, "%s", description);
	qvcam_driver_register();

	return 0;
}

void qvcam_driver_uninit(void)
{
	qpr_function();

	if (!qvcam_driver_global)
		return;

	qvcam_driver_unregister();
	kfree(qvcam_driver_global);
	qvcam_driver_global = NULL;
}

const char *qvcam_driver_name(void)
{
	if (!qvcam_driver_global)
		return NULL;

	return qvcam_driver_global->name;
}

const char *qvcam_driver_description(void)
{
	if (!qvcam_driver_global)
		return NULL;

	return qvcam_driver_global->description;
}

uint qvcam_driver_version(void)
{
	return LINUX_VERSION_CODE;
}

bool qvcam_driver_register(void)
{
	qpr_function();

    return true;
}

void qvcam_driver_unregister(void)
{
	qpr_function();
}
