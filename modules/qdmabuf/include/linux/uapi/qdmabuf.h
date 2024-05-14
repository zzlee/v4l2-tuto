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

/**
 * struct qdmabuf_allocation_data - metadata passed from userspace for
 *                                      allocations
 *
 * Provided by userspace as an argument to the ioctl
 */
struct qdmabuf_allocation_data {
	__u64 len;
	__u32 fd;
};

#define QDMABUF_IOC_MAGIC		'Q'

/**
 * DOC: QDMABU_IOCTL_ALLOC - allocate memory from pool
 *
 */
#define QDMABU_IOCTL_ALLOC	_IOWR(QDMABUF_IOC_MAGIC, 0x0, struct qdmabuf_allocation_data)

#endif /* _UAPI_LINUX_QDMABUF_H */