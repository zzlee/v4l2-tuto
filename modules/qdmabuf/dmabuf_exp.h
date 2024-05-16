#ifndef __QDMABUF_DMABUF_EXP_H__
#define __QDMABUF_DMABUF_EXP_H__

#include <linux/device.h>
#include <linux/refcount.h>
#include <linux/mm.h>

struct dmabuf_exp_vmarea_handler {
	refcount_t *refcount;
	void (*put)(void *arg);
	void *arg;
};

extern const struct vm_operations_struct dmabuf_exp_vm_ops;

int qdmabuf_dmabuf_alloc_contig(struct device* device, int len, int fd_flags);
int qdmabuf_dmabuf_alloc_sg(struct device* device, int len, int fd_flags);
int qdmabuf_dmabuf_alloc_vmalloc(struct device* device, int len, int fd_flags);

#endif // __QDMABUF_DMABUF_EXP_H__