/*
 * QDMABUF Userspace API
 */
#ifndef _UAPI_LINUX_QDMABUF_H
#define _UAPI_LINUX_QDMABUF_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * DOC: QDMABUF Userspace API
 */

#define QDMABUF_ALLOC_TYPE_CONTIG	0x00
#define QDMABUF_ALLOC_TYPE_SG		0x01

#define QDMABUF_ALLOC_FD_FLAGS (O_CLOEXEC | O_ACCMODE)

/**
 * struct qdmabuf_alloc_args - metadata passed from userspace for
 *                                      allocations
 *
 * Provided by userspace as an argument to the ioctl
 */
struct qdmabuf_alloc_args {
	__u64 len;
	__u32 type;
	__u32 fd_flags;
	__u32 fd;
};

struct qdmabuf_info_args {
	__u32 fd;
};

#define QDMABUF_IOC_MAGIC		'Q'

/**
 * DOC: QDMABUF_IOCTL_ALLOC - allocate memory from pool
 *
 */
#define QDMABUF_IOCTL_ALLOC		_IOWR(QDMABUF_IOC_MAGIC, 0x0, struct qdmabuf_alloc_args)
#define QDMABUF_IOCTL_INFO		_IOWR(QDMABUF_IOC_MAGIC, 0x1, struct qdmabuf_info_args)

#endif /* _UAPI_LINUX_QDMABUF_H */