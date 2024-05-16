#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s: " fmt, __func__

#include "dmabuf_exp.h"

#include <linux/version.h>
#include <linux/err.h>
#include <linux/slab.h>

struct exp_sg_buffer {
	struct device *dev;
	void *vaddr;
	unsigned long size;
	struct page **pages;
	struct sg_table sg_table;

	struct dmabuf_exp_vmarea_handler handler;
	refcount_t refcount;
	struct sg_table *dma_sgt;
	unsigned int num_pages;

	struct dma_buf_attachment *db_attach;
};

struct exp_sg_attachment {
	struct sg_table sgt;
	enum dma_data_direction dma_dir;
};

static void exp_sg_buffer_put(void *buf_priv)
{
	struct exp_sg_buffer *buf = buf_priv;

	if (!refcount_dec_and_test(&buf->refcount))
		return;

	struct sg_table *sgt = &buf->sg_table;
	int i = buf->num_pages;

	dprintk(1, "%s: Freeing buffer of %d pages\n", __func__,
		buf->num_pages);
	dma_unmap_sg_attrs(buf->dev, sgt->sgl, sgt->orig_nents,
		buf->dma_dir, DMA_ATTR_SKIP_CPU_SYNC);
	if (buf->vaddr)
		vm_unmap_ram(buf->vaddr, buf->num_pages);
	sg_free_table(buf->dma_sgt);
	while (--i >= 0)
		__free_page(buf->pages[i]);
	kvfree(buf->pages);
	put_device(buf->dev);
	kfree(buf);
}

static int exp_sg_attach(struct dma_buf *dbuf, struct dma_buf_attachment *dbuf_attach) {
	struct exp_sg_attachment *attach;
	unsigned int i;
	struct scatterlist *rd, *wr;
	struct sg_table *sgt;
	struct exp_contig_buffer *buf = dbuf->priv;
	int ret;

	pr_info("%s(#%d): buf=%p\n", __func__, __LINE__, buf);

	attach = kzalloc(sizeof(*attach), GFP_KERNEL);
	if (!attach) {
		pr_err("%s(#%d): kzalloc() failed\n", __func__, __LINE__);

		ret = -ENOMEM;
		goto err0;
	}

	sgt = &attach->sgt;
	/* Copy the buf->base_sgt scatter list to the attachment, as we can't
	 * map the same scatter list to multiple attachments at the same time.
	 */
	ret = sg_alloc_table(sgt, buf->dma_sgt->orig_nents, GFP_KERNEL);
	if (ret) {
		pr_err("%s(#%d): sg_alloc_table() failed, err=%d\n", __func__, __LINE__, ret);

		goto err1;
	}

	pr_info("%s(#%d): sgt->orig_nents=%d\n", __func__, __LINE__, sgt->orig_nents);

	rd = buf->dma_sgt->sgl;
	wr = sgt->sgl;
	for (i = 0; i < sgt->orig_nents; ++i) {
		sg_set_page(wr, sg_page(rd), rd->length, rd->offset);
		rd = sg_next(rd);
		wr = sg_next(wr);
	}

	attach->dma_dir = DMA_NONE;
	dbuf_attach->priv = attach;

	return 0;

err1:
	kfree(attach);
err0:
	return ret;
}

static void exp_sg_detach(struct dma_buf *dbuf, struct dma_buf_attachment *db_attach) {
	struct exp_sg_attachment *attach = db_attach->priv;
	struct sg_table *sgt;

	pr_info("%s(#%d): attach=%p\n", __func__, __LINE__, attach);

	if (!attach) {
		pr_err("%s(#%d): unexpected value, attach=%p\n", __func__, __LINE__, attach);
		goto err0;
	}

	sgt = &attach->sgt;

	/* release the scatterlist cache */
	if (attach->dma_dir != DMA_NONE) {
		/*
		 * Cache sync can be skipped here, as the exporter memory is
		 * allocated from device coherent memory, which means the
		 * memory locations do not require any explicit cache
		 * maintenance prior or after being used by the device.
		 */
		dma_unmap_sg_attrs(db_attach->dev, sgt->sgl, sgt->orig_nents, attach->dma_dir, DMA_ATTR_SKIP_CPU_SYNC);
	}
	sg_free_table(sgt);
	kfree(attach);
	db_attach->priv = NULL;

	return;

err0:
	return;
}

static void exp_sg_dma_buf_release(struct dma_buf *dbuf) {
	struct exp_sg_buffer *buf = dbuf->priv;

	pr_info("%s(#%d): buf=%p\n", __func__, __LINE__, buf);

	exp_sg_buffer_put(dbuf->priv);
}

static struct sg_table * exp_sg_map_dma_buf(struct dma_buf_attachment *db_attach,
	enum dma_data_direction dma_dir) {
	struct exp_sg_attachment *attach = db_attach->priv;
	/* stealing dmabuf mutex to serialize map/unmap operations */
	struct mutex *lock = &db_attach->dmabuf->lock;
	struct sg_table *sgt;
	int err;

	pr_info("%s(#%d): db_attach=%p\n", __func__, __LINE__, db_attach);

	if (!attach) {
		pr_err("%s(#%d): unexpected value, attach=%p\n", __func__, __LINE__, attach);

		sgt = NULL;
		goto err0;
	}

	mutex_lock(lock);

	sgt = &attach->sgt;
	/* return previously mapped sg table */
	if (attach->dma_dir == dma_dir) {
		goto done;
	}

	/* release any previous cache */
	if (attach->dma_dir != DMA_NONE) {
		dma_unmap_sgtable(db_attach->dev, sgt, attach->dma_dir, DMA_ATTR_SKIP_CPU_SYNC);
		attach->dma_dir = DMA_NONE;
	}

	err = dma_map_sgtable(db_attach->dev, sgt, dma_dir, DMA_ATTR_SKIP_CPU_SYNC);
	if(err) {
		pr_err("%s(#%d): dma_map_sgtable() failed, err=%d\n", __func__, __LINE__, err);
		sgt = ERR_PTR(-EIO);
		goto err1;
	}

	attach->dma_dir = dma_dir;

done:
	mutex_unlock(lock);
	return sgt;

err1:
	mutex_unlock(lock);
err0:
	return sgt;
}

static void exp_contig_unmap_dma_buf(struct dma_buf_attachment * db_attach,
	struct sg_table * sgt,
	enum dma_data_direction dma_dir) {

	pr_info("%s(#%d): db_attach=%p\n", __func__, __LINE__, db_attach);

	/* nothing to be done here */
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,9,0)
static void * exp_contig_vmap(struct dma_buf *dbuf)
#else
static int exp_contig_vmap(struct dma_buf *dbuf, struct dma_buf_map *map)
#endif
{
	struct exp_contig_buffer *buf = dbuf->priv;

	pr_info("%s(#%d): buf=%p\n", __func__, __LINE__, buf);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,9,0)
	return buf->vaddr;
#else
	dma_buf_map_set_vaddr(map, buf->vaddr);

	return 0;
#endif
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,9,0)
static void exp_contig_vunmap(struct dma_buf *dbuf, void *vaddr)
#else
static void exp_contig_vunmap(struct dma_buf *dbuf, struct dma_buf_map *map)
#endif
{
	struct exp_contig_buffer *buf = dbuf->priv;

	pr_info("%s(#%d): buf=%p\n", __func__, __LINE__, buf);
}

static int exp_contig_mmap(struct dma_buf *dbuf,
	struct vm_area_struct *vma) {
	struct exp_contig_buffer *buf = dbuf->priv;
	int ret;

	pr_info("%s(#%d): buf=%p\n", __func__, __LINE__, buf);

	if (!buf) {
		pr_err("%s(#%d): unexpected value, buf=%p\n", __func__, __LINE__, buf);
		ret = -EINVAL;
		goto err0;
	}

	ret = dma_mmap_attrs(buf->dev, vma, buf->vaddr,
		buf->dma_addr, buf->size, buf->attrs);
	if (ret) {
		pr_err("%s(#%d): dma_mmap_attrs() failed, err=%d\n", __func__, __LINE__, ret);
		goto err0;
	}

	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = &buf->handler;
	vma->vm_ops = &dmabuf_exp_vm_ops;

	vma->vm_ops->open(vma);

	pr_debug("%s: mapped dma addr 0x%08lx at 0x%08lx, size %ld\n",
		__func__, (unsigned long)buf->dma_addr, vma->vm_start,
		buf->size);

	return 0;

err0:
	return ret;
}

static const struct dma_buf_ops exp_sg_buf_ops = {
	.attach = exp_sg_attach,
	.detach = exp_sg_detach,
	.map_dma_buf = exp_sg_map_dma_buf,
	.unmap_dma_buf = exp_sg_unmap_dma_buf,
	.release = exp_sg_dma_buf_release,
	.map = exp_sg_map,
	.mmap = exp_sg_mmap,
	.vmap = exp_sg_vmap,
	.vunmap = exp_sg_vunmap,
};

int qdmabuf_dmabuf_alloc_contig(struct device* device, int len, int fd_flags) {
	struct exp_contig_buffer *buf;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	size_t size = PAGE_ALIGN(len);
	struct dma_buf *dmabuf;
	int ret;

	pr_info("%s(#%d): len=%d, fd_flags=%d\n", __func__, __LINE__, len, fd_flags);

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf) {
		pr_err("%s(#%d): kzalloc() failed\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto err0;
	}

	buf->dev = get_device(device);
	if(!buf->dev) {
		pr_err("%s(#%d): get_device() failed\n", __func__, __LINE__);
		ret = -EINVAL;
		goto err1;
	}

	buf->size = size;
	buf->attrs = 0;

	buf->handler.refcount = &buf->refcount;
	buf->handler.put = exp_contig_buffer_put;
	buf->handler.arg = buf;
	refcount_set(&buf->refcount, 1);

	buf->vaddr = dma_alloc_attrs(buf->dev, buf->size, &buf->dma_addr, GFP_KERNEL, buf->attrs);
	if (!buf->vaddr) {
		pr_err("%s(#%d): dma_alloc_attrs() failed\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto err2;
	}

	pr_info("buf={.size=%d, .dev=%p .vaddr=%p, .dma_addr=%p\n",
		(int)buf->size, buf->dev, (void*)buf->vaddr, (void*)buf->dma_addr);

	buf->sgt_base = kmalloc(sizeof(*buf->sgt_base), GFP_KERNEL);
	if (!buf->sgt_base) {
		pr_err("%s(#%d): kmalloc() failed\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto err3;
	}

	ret = dma_get_sgtable_attrs(buf->dev, buf->sgt_base, buf->vaddr, buf->dma_addr, buf->size, buf->attrs);
	if (ret < 0) {
		pr_err("%s(#%d): dma_get_sgtable_attrs() failed, err=%d\n", __func__, __LINE__, ret);
		goto err4;
	}

	exp_info.exp_name = "qdmabuf";
	exp_info.ops = &exp_contig_buf_ops;
	exp_info.size = buf->size;
	exp_info.flags = fd_flags;
	exp_info.priv = buf;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		pr_err("%s(#%d): dma_buf_export() failed, dmabuf=%p\n", __func__, __LINE__, dmabuf);

		ret = PTR_ERR(dmabuf);
		goto err5;
	}

	ret = dma_buf_fd(dmabuf, fd_flags);
	if (ret < 0) {
		pr_err("%s(#%d): dma_buf_fd() failed, err=%d\n", __func__, __LINE__, ret);

		goto err6;
	}

	return ret;

err6:
	dma_buf_put(dmabuf);
err5:
	sg_free_table(buf->sgt_base);
err4:
	kfree(buf->sgt_base);
err3:
	dma_free_attrs(buf->dev, buf->size, buf->vaddr, buf->dma_addr, buf->attrs);
err2:
	put_device(buf->dev);
err1:
	kfree(buf);
err0:
	return ret;
}
