#ifndef QVCAM_DRIVER_H
#define QVCAM_DRIVER_H

#include <linux/types.h>

int qvcam_driver_init(const char *name, const char *description);
void qvcam_driver_uninit(void);

const char *qvcam_driver_name(void);
const char *qvcam_driver_description(void);
uint qvcam_driver_version(void);

#endif // QVCAM_DRIVER_H
