#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "log.h"
#include "driver.h"

#define QVCAM_DRIVER_NAME        "qvcam"
#define QVCAM_DRIVER_DESCRIPTION "QCAP Virtual Camera"

static int loglevel = 0;
module_param(loglevel, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(loglevel, "Debug verbosity (-2 to 7)");

static int __init qvcam_init(void)
{
    qvcam_log_set_level(loglevel);

    return qvcam_driver_init(QVCAM_DRIVER_NAME, QVCAM_DRIVER_DESCRIPTION);
}

static void __exit qvcam_uninit(void)
{
    qvcam_driver_uninit();
}

module_init(qvcam_init)
module_exit(qvcam_uninit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zzlab");
MODULE_DESCRIPTION(QVCAM_DRIVER_DESCRIPTION);
MODULE_VERSION(QVCAM_MODULE_VERSION);
