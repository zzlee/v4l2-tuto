/* akvcam, virtual camera for Linux.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef QVCAM_LOG_H
#define QVCAM_LOG_H

#include <linux/version.h>

#define qpr_file_name (strrchr(__FILE__, '/') + 1)
#define qpr_log_format "[qvcam] %s(%d): "

#define qpr_err(fmt, ...) \
    do { \
        if (qvcam_log_level() >= LOGLEVEL_ERR) { \
            printk(KERN_ERR qpr_log_format fmt, \
                   qpr_file_name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (false)

#define qpr_warning(fmt, ...) \
    do { \
        if (qvcam_log_level() >= LOGLEVEL_WARNING) { \
            printk(KERN_WARNING qpr_log_format fmt, \
                   qpr_file_name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (false)

#define qpr_info(fmt, ...) \
    do { \
        if (qvcam_log_level() >= LOGLEVEL_INFO) { \
            printk(KERN_INFO qpr_log_format fmt, \
                   qpr_file_name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (false)

#define qpr_debug(fmt, ...) \
    do { \
        if (qvcam_log_level() >= LOGLEVEL_DEBUG) { \
            printk(KERN_DEBUG qpr_log_format fmt, \
                   qpr_file_name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (false)

#define qpr_function() \
    qpr_debug("%s()\n", __FUNCTION__)

int qvcam_log_level(void);
void qvcam_log_set_level(int level);

#endif // QVCAM_LOG_H
