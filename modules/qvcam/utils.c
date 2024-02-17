#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>

#include "utils.h"

static struct qvcam_utils
{
	uint64_t id;
	int last_error;
} qvcam_utils_private;

typedef struct
{
	__u32 cmd;
	char str[32];
} qvcam_utils_ioctl_strings, *qvcam_utils_ioctl_strings_t;

typedef struct
{
	int error;
	char str[32];
	char description[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_error_strings, *qvcam_utils_error_strings_t;

typedef struct
{
	enum v4l2_buf_type type;
	char  str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_buf_type_strings, *qvcam_utils_buf_type_strings_t;

typedef struct
{
	enum v4l2_field field;
	char str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_field_strings, *qvcam_utils_field_strings_t;

typedef struct
{
	enum v4l2_frmsizetypes type;
	char str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_frmsize_type_strings, *qvcam_utils_frmsize_type_strings_t;

typedef struct
{
	__u32 pixelformat;
	char str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_pixelformat_strings, *qvcam_utils_pixelformat_strings_t;

typedef struct
{
	enum v4l2_memory memory;
	char str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_v4l2_memory_strings, *qvcam_utils_v4l2_memory_strings_t;

typedef struct
{
	enum v4l2_colorspace colorspace;
	char str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_v4l2_colorspace_strings, *qvcam_utils_v4l2_colorspace_strings_t;

typedef struct
{
	__u32 flag;
	char str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_buffer_flags_strings, *qvcam_utils_buffer_flags_strings_t;

typedef struct
{
	__u32 flag;
	char str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_buffer_capabilities_strings, *qvcam_utils_buffer_capabilities_strings_t;

typedef struct
{
	__s32 ctrl_which;
	char str[QVCAM_MAX_STRING_SIZE];
} qvcam_utils_ctrl_which_class_strings, *qvcam_utils_ctrl_which_class_strings_t;

uint64_t qvcam_id(void)
{
	return qvcam_utils_private.id++;
}

int qvcam_get_last_error(void)
{
	return qvcam_utils_private.last_error;
}

int qvcam_set_last_error(int error)
{
	qvcam_utils_private.last_error = error;

	return error;
}

void qvcam_string_from_error(int error, char *str, size_t len)
{
	size_t i;
	static const qvcam_utils_error_strings error_strings[] = {
		{EPERM	, "EPERM"  , "Operation not permitted"            },
		{ENOENT	, "ENOENT" , "No such file or directory"          },
		{ESRCH	, "ESRCH"  , "No such process"                    },
		{EINTR	, "EINTR"  , "Interrupted system call"            },
		{EIO	, "EIO"    , "I/O error"                          },
		{ENXIO	, "ENXIO"  , "No such device or address"          },
		{E2BIG	, "E2BIG"  , "Argument list too long"             },
		{ENOEXEC, "ENOEXEC", "Exec format error"                  },
		{EBADF	, "EBADF"  , "Bad file number"                    },
		{ECHILD	, "ECHILD" , "No child processes"                 },
		{EAGAIN	, "EAGAIN" , "Try again"                          },
		{ENOMEM	, "ENOMEM" , "Out of memory"                      },
		{EACCES	, "EACCES" , "Permission denied"                  },
		{EFAULT	, "EFAULT" , "Bad address"                        },
		{ENOTBLK, "ENOTBLK", "Block device required"              },
		{EBUSY	, "EBUSY"  , "Device or resource busy"            },
		{EEXIST	, "EEXIST" , "File exists"                        },
		{EXDEV	, "EXDEV"  , "Cross-device link"                  },
		{ENODEV	, "ENODEV" , "No such device"                     },
		{ENOTDIR, "ENOTDIR", "Not a directory"                    },
		{EISDIR	, "EISDIR" , "Is a directory"                     },
		{EINVAL	, "EINVAL" , "Invalid argument"                   },
		{ENFILE	, "ENFILE" , "File table overflow"                },
		{EMFILE	, "EMFILE" , "Too many open files"                },
		{ENOTTY	, "ENOTTY" , "Not a typewriter"                   },
		{ETXTBSY, "ETXTBSY", "Text file busy"                     },
		{EFBIG	, "EFBIG"  , "File too large"                     },
		{ENOSPC	, "ENOSPC" , "No space left on device"            },
		{ESPIPE	, "ESPIPE" , "Illegal seek"                       },
		{EROFS	, "EROFS"  , "Read-only file system "             },
		{EMLINK	, "EMLINK" , "Too many links"                     },
		{EPIPE	, "EPIPE"  , "Broken pipe"                        },
		{EDOM	, "EDOM"   , "Math argument out of domain of func"},
		{ERANGE	, "ERANGE" , "Math result not representable"      },
		{0      , ""       , ""                                   },
	};

	memset(str, 0, len);

	for (i = 0; qvcam_strlen(error_strings[i].str) > 0; i++)
		if (error_strings[i].error == -error) {
			snprintf(str,
					 len,
					 "%s (%s)",
					 error_strings[i].description,
					 error_strings[i].str);

			return;
		}

	snprintf(str, len, "Unknown error (%d)", error);
}

char *qvcam_strdup(const char *str, QVCAM_MEMORY_TYPE type)
{
	char *str_dup;
	size_t len = qvcam_strlen(str);

	if (type == QVCAM_MEMORY_TYPE_KMALLOC)
		str_dup = kmalloc(len + 1, GFP_KERNEL);
	else
		str_dup = vmalloc(len + 1);

	str_dup[len] = 0;

	if (str)
		memcpy(str_dup, str, len + 1);

	return str_dup;
}

char *qvcam_strip_str(const char *str, QVCAM_MEMORY_TYPE type)
{
	if (!str)
		return NULL;

	return qvcam_strip_str_sub(str,
								0,
								qvcam_strlen(str),
								type);
}

char *qvcam_strip_str_sub(const char *str,
						   size_t from,
						   size_t size,
						   QVCAM_MEMORY_TYPE type)
{
	char *stripped_str;
	ssize_t i;
	size_t len;
	size_t left;
	size_t stripped_len;

	len = qvcam_min(qvcam_strlen(str), from + size);

	for (i = (ssize_t) from; i < (ssize_t) len; i++)
		if (!isspace(str[i]))
			break;

	left = (size_t) i;

	if (left == len) {
		stripped_len = 0;
	} else {
		size_t right;

		for (i = (ssize_t) len - 1; i >= (ssize_t) from; i--)
			if (!isspace(str[i]))
				break;

		right = (size_t) i;
		stripped_len = right - left + 1;
	}

	if (type == QVCAM_MEMORY_TYPE_KMALLOC)
		stripped_str = kmalloc(stripped_len + 1, GFP_KERNEL);
	else
		stripped_str = vmalloc(stripped_len + 1);

	stripped_str[stripped_len] = 0;

	if (stripped_len > 0)
		memcpy(stripped_str, str + left, stripped_len);

	return stripped_str;
}

void qvcam_replace(char *str, char from, char to)
{
	if (!str)
		return;

	for (; *str; str++)
		if (*str == from)
			*str = to;
}
