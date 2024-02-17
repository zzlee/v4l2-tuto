#include "log.h"

static struct qvcam_log
{
    int level;
} qvcam_log_private;

int qvcam_log_level(void)
{
    return qvcam_log_private.level;
}

void qvcam_log_set_level(int level)
{
    qvcam_log_private.level = level;
}
