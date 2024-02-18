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

#include <linux/slab.h>
#include <media/v4l2-dev.h>

#include "attributes.h"
#include "controls.h"
#include "device.h"
#include "list.h"

static const struct attribute_group *akvcam_attributes_capture_groups[2];
static const struct attribute_group *akvcam_attributes_output_groups[2];

typedef struct
{
    AKVCAM_RW_MODE rw_mode;
    char  str[AKVCAM_MAX_STRING_SIZE];
} akvcam_attributes_rw_mode_strings, *akvcam_attributes_rw_mode_strings_t;

typedef struct
{
    const char *name;
    __u32 id;
} akvcam_attributes_controls_map, *akvcam_attributes_controls_map_t;

static akvcam_attributes_controls_map akvcam_attributes_controls[] = {
    {"brightness"  , V4L2_CID_BRIGHTNESS    },
    {"contrast"    , V4L2_CID_CONTRAST      },
    {"saturation"  , V4L2_CID_SATURATION    },
    {"hue"         , V4L2_CID_HUE           },
    {"gamma"       , V4L2_CID_GAMMA         },
    {"hflip"       , V4L2_CID_HFLIP         },
    {"vflip"       , V4L2_CID_VFLIP         },
    {"scaling"     , AKVCAM_CID_SCALING     },
    {"aspect_ratio", AKVCAM_CID_ASPECT_RATIO},
    {"swap_rgb"    , AKVCAM_CID_SWAP_RGB    },
    {"colorfx"     , V4L2_CID_COLORFX       },
    {NULL          , 0                      },
};

__u32 akvcam_attributes_controls_id_by_name(const char *name);

const struct attribute_group **akvcam_attributes_groups(AKVCAM_DEVICE_TYPE device_type)
{
    return device_type == AKVCAM_DEVICE_TYPE_OUTPUT?
                akvcam_attributes_output_groups:
                akvcam_attributes_capture_groups;
}

__u32 akvcam_attributes_controls_id_by_name(const char *name)
{
    size_t i;

    for (i = 0; akvcam_attributes_controls[i].name; i++)
        if (strcmp(akvcam_attributes_controls[i].name, name) == 0)
            return akvcam_attributes_controls[i].id;

    return 0;
}

static ssize_t akvcam_attributes_connected_devices_show(struct device *dev,
                                                        struct device_attribute *attribute,
                                                        char *buffer)
{
    struct video_device *vdev = to_video_device(dev);
    akvcam_device_t device = video_get_drvdata(vdev);
    akvcam_devices_list_t devices;
    akvcam_list_element_t it = NULL;
    size_t n = 0;
    size_t i;

    UNUSED(attribute);
    devices = akvcam_device_connected_devices_nr(device);
    memset(buffer, 0, PAGE_SIZE);

    for (i = 0; i < 64 && PAGE_SIZE > n; i++) {
        device = akvcam_list_next(devices, &it);

        if (!it)
            break;

        n = snprintf(buffer + n,
                     PAGE_SIZE - n,
                     "/dev/video%d\n",
                     akvcam_device_num(device));
    }

    return n;
}

static ssize_t akvcam_attributes_streaming_devices_show(struct device *dev,
                                                        struct device_attribute *attribute,
                                                        char *buffer)
{
    struct video_device *vdev = to_video_device(dev);
    akvcam_device_t device = video_get_drvdata(vdev);
    akvcam_devices_list_t devices;
    akvcam_list_element_t it = NULL;
    size_t n = 0;
    size_t i;

    UNUSED(attribute);
    devices = akvcam_device_connected_devices_nr(device);
    memset(buffer, 0, PAGE_SIZE);

    for (i = 0; i < 64;) {
        device = akvcam_list_next(devices, &it);

        if (!it)
            break;

        if (akvcam_device_streaming(device)) {
            n = snprintf(buffer + n,
                         PAGE_SIZE - n,
                         "/dev/video%d\n",
                         akvcam_device_num(device));
            i++;
        }
    }

    return n;
}

static ssize_t akvcam_attributes_device_modes_show(struct device *dev,
                                                   struct device_attribute *attribute,
                                                   char *buffer)
{
    struct video_device *vdev = to_video_device(dev);
    akvcam_device_t device = video_get_drvdata(vdev);
    AKVCAM_RW_MODE rw_mode = akvcam_device_rw_mode(device);
    char *data = buffer;
    size_t i;
    size_t n = 0;
    static const akvcam_attributes_rw_mode_strings rw_mode_strings[] = {
        {AKVCAM_RW_MODE_READWRITE, "rw"     },
        {AKVCAM_RW_MODE_MMAP     , "mmap"   },
        {AKVCAM_RW_MODE_USERPTR  , "userptr"},
        {AKVCAM_RW_MODE_DMABUF   , "dmabuf" },
        {0                       , ""       },
    };

    UNUSED(attribute);
    memset(data, 0, PAGE_SIZE);

    for (i = 0; akvcam_strlen(rw_mode_strings[i].str) > 0; i++)
        if (rw_mode_strings[i].rw_mode & rw_mode)
            n += snprintf(data + n, PAGE_SIZE - n, "%s\n", rw_mode_strings[i].str);

    return (ssize_t) n;
}


static ssize_t akvcam_attributes_int_show(struct device *dev,
                                          struct device_attribute *attribute,
                                          char *buffer)
{
    struct video_device *vdev = to_video_device(dev);
    akvcam_device_t device = video_get_drvdata(vdev);
    akvcam_controls_t controls;
    __u32 id = akvcam_attributes_controls_id_by_name(attribute->attr.name);
    __s32 value;

    controls = akvcam_device_controls_nr(device);
    value = akvcam_controls_value(controls, id);
    memset(buffer, 0, PAGE_SIZE);

    return snprintf(buffer, PAGE_SIZE, "%d\n", value);
}

static ssize_t akvcam_attributes_int_store(struct device *dev,
                                           struct device_attribute *attribute,
                                           const char *buffer,
                                           size_t size)
{
    struct video_device *vdev = to_video_device(dev);
    akvcam_device_t device = video_get_drvdata(vdev);
    akvcam_controls_t controls;
    __u32 id = akvcam_attributes_controls_id_by_name(attribute->attr.name);
    __s32 value = 0;
    int result;

    if (kstrtos32(buffer, 10, (__s32 *) &value) != 0)
        return -EINVAL;

    controls = akvcam_device_controls_nr(device);
    result = akvcam_controls_set_value(controls, id, value);

    if (result)
        return result;

    return (ssize_t) size;
}

static ssize_t akvcam_attributes_menu_show(struct device *dev,
                                           struct device_attribute *attribute,
                                           char *buffer)
{
    struct video_device *vdev = to_video_device(dev);
    akvcam_device_t device = video_get_drvdata(vdev);
    akvcam_controls_t controls;
    __u32 id = akvcam_attributes_controls_id_by_name(attribute->attr.name);
    const char *value;

    controls = akvcam_device_controls_nr(device);
    value = akvcam_controls_string_value(controls, id);
    memset(buffer, 0, PAGE_SIZE);

    return snprintf(buffer, PAGE_SIZE, "%s\n", value);
}

static ssize_t akvcam_attributes_menu_store(struct device *dev,
                                            struct device_attribute *attribute,
                                            const char *buffer,
                                            size_t size)
{
    struct video_device *vdev = to_video_device(dev);
    akvcam_device_t device = video_get_drvdata(vdev);
    akvcam_controls_t controls;
    char *buffer_stripped;
    __u32 id = akvcam_attributes_controls_id_by_name(attribute->attr.name);
    ssize_t result;

    controls = akvcam_device_controls_nr(device);
    buffer_stripped = akvcam_strip_str(buffer, AKVCAM_MEMORY_TYPE_KMALLOC);
    result = akvcam_controls_set_string_value(controls, id, buffer_stripped);
    kfree(buffer_stripped);

    if (result)
        return result;

    return size;
}

static DEVICE_ATTR(connected_devices,
                   S_IRUGO,
                   akvcam_attributes_connected_devices_show,
                   NULL);
static DEVICE_ATTR(listeners,
                   S_IRUGO,
                   akvcam_attributes_streaming_devices_show,
                   NULL);
static DEVICE_ATTR(broadcasters,
                   S_IRUGO,
                   akvcam_attributes_streaming_devices_show,
                   NULL);
static DEVICE_ATTR(modes,
                   S_IRUGO,
                   akvcam_attributes_device_modes_show,
                   NULL);
static DEVICE_ATTR(brightness,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_int_show,
                   akvcam_attributes_int_store);
static DEVICE_ATTR(contrast,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_int_show,
                   akvcam_attributes_int_store);
static DEVICE_ATTR(saturation,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_int_show,
                   akvcam_attributes_int_store);
static DEVICE_ATTR(hue,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_int_show,
                   akvcam_attributes_int_store);
static DEVICE_ATTR(gamma,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_int_show,
                   akvcam_attributes_int_store);
static DEVICE_ATTR(hflip,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_int_show,
                   akvcam_attributes_int_store);
static DEVICE_ATTR(colorfx,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_menu_show,
                   akvcam_attributes_menu_store);
static DEVICE_ATTR(vflip,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_int_show,
                   akvcam_attributes_int_store);
static DEVICE_ATTR(aspect_ratio,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_menu_show,
                   akvcam_attributes_menu_store);
static DEVICE_ATTR(scaling,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_menu_show,
                   akvcam_attributes_menu_store);
static DEVICE_ATTR(swap_rgb,
                   S_IRUGO | S_IWUSR,
                   akvcam_attributes_int_show,
                   akvcam_attributes_int_store);

// Define capture groups.

static struct attribute	*akvcam_attributes_capture[] = {
    &dev_attr_connected_devices.attr,
    &dev_attr_broadcasters.attr,
    &dev_attr_modes.attr,
    &dev_attr_brightness.attr,
    &dev_attr_contrast.attr,
    &dev_attr_saturation.attr,
    &dev_attr_hue.attr,
    &dev_attr_gamma.attr,
    &dev_attr_hflip.attr,
    &dev_attr_vflip.attr,
    &dev_attr_colorfx.attr,
    NULL
};

static struct attribute_group akvcam_attributes_capture_group = {
    .name = "controls",
    .attrs = akvcam_attributes_capture
};

static const struct attribute_group *akvcam_attributes_capture_groups[] = {
    &akvcam_attributes_capture_group,
    NULL
};

// Define output groups.

static struct attribute	*akvcam_attributes_output[] = {
    &dev_attr_connected_devices.attr,
    &dev_attr_listeners.attr,
    &dev_attr_modes.attr,
    &dev_attr_hflip.attr,
    &dev_attr_vflip.attr,
    &dev_attr_aspect_ratio.attr,
    &dev_attr_scaling.attr,
    &dev_attr_swap_rgb.attr,
    NULL
};

static struct attribute_group akvcam_attributes_output_group = {
    .name = "controls",
    .attrs = akvcam_attributes_output
};

static const struct attribute_group *akvcam_attributes_output_groups[] = {
    &akvcam_attributes_output_group,
    NULL
};
