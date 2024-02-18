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

#ifndef AKVCAM_LOG_H
#define AKVCAM_LOG_H

#include <linux/version.h>

#define akpr_file_name (strrchr(__FILE__, '/') + 1)
#define akpr_log_format "[akvcam] %s(%d): "

#define akpr_err(fmt, ...) \
    do { \
        if (akvcam_log_level() >= LOGLEVEL_ERR) { \
            printk(KERN_ERR akpr_log_format fmt, \
                   akpr_file_name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (false)

#define akpr_warning(fmt, ...) \
    do { \
        if (akvcam_log_level() >= LOGLEVEL_WARNING) { \
            printk(KERN_WARNING akpr_log_format fmt, \
                   akpr_file_name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (false)

#define akpr_info(fmt, ...) \
    do { \
        if (akvcam_log_level() >= LOGLEVEL_INFO) { \
            printk(KERN_INFO akpr_log_format fmt, \
                   akpr_file_name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (false)

#define akpr_debug(fmt, ...) \
    do { \
        if (akvcam_log_level() >= LOGLEVEL_DEBUG) { \
            printk(KERN_DEBUG akpr_log_format fmt, \
                   akpr_file_name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (false)

#define akpr_function() \
    akpr_debug("%s()\n", __FUNCTION__)

int akvcam_log_level(void);
void akvcam_log_set_level(int level);

#endif // AKVCAM_LOG_H
