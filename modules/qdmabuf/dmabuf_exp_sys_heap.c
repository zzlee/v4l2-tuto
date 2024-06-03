#define pr_fmt(fmt)     "[" KBUILD_MODNAME "]%s(#%d): " fmt, __func__, __LINE__

#include "dmabuf_exp.h"

#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/highmem.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,9,0)
#else
struct exp_sys_heap_buffer {
	struct list_head attachments;
	struct mutex lock;
	unsigned long len;
	struct sg_table sg_table;
	int vmap_cnt;
	void *vaddr;
};

struct exp_sys_heap_attachment {
	struct device *dev;
	struct sg_table *table;
	struct list_head list;
	bool mapped;
};

#define HIGH_ORDER_GFP  (((GFP_HIGHUSER | __GFP_ZERO | __GFP_NOWARN \
				| __GFP_NORETRY) & ~__GFP_RECLAIM) \
				| __GFP_COMP)
#define LOW_ORDER_GFP (GFP_HIGHUSER | __GFP_ZERO | __GFP_COMP)
static gfp_t order_flags[] = {HIGH_ORDER_GFP, LOW_ORDER_GFP, LOW_ORDER_GFP};
/*
 * The selection of the orders used for allocation (1MB, 64K, 4K) is designed
 * to match with the sizes often found in IOMMUs. Using order 4 pages instead
 * of order 0 pages can significantly improve the performance of many IOMMUs
 * by reducing TLB pressure and time spent updating page tables.
 */
static const unsigned int orders[] = {8, 4, 0};
#define NUM_ORDERS ARRAY_SIZE(orders)

static struct sg_table *dup_sg_table(struct sg_table *table)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return ERR_PTR(-ENOMEM);

	ret = sg_alloc_table(new_table, table->orig_nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return ERR_PTR(-ENOMEM);
	}

	new_sg = new_table->sgl;
	for_each_sgtable_sg(table, sg, i) {
		sg_set_page(new_sg, sg_page(sg), sg->length, sg->offset);
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static int exp_sys_heap_attach(struct dma_buf *dmabuf, struct dma_buf_attachment *attachment) {
	struct exp_sys_heap_buffer *buffer = dmabuf->priv;
	struct exp_sys_heap_attachment *a;
	struct sg_table *table;

	a = kzalloc(sizeof(*a), GFP_KERNEL);
	if (!a)
		return -ENOMEM;

	table = dup_sg_table(&buffer->sg_table);
	if (IS_ERR(table)) {
		kfree(a);
		return -ENOMEM;
	}

	a->table = table;
	a->dev = attachment->dev;
	INIT_LIST_HEAD(&a->list);
	a->mapped = false;

	attachment->priv = a;

	mutex_lock(&buffer->lock);
	list_add(&a->list, &buffer->attachments);
	mutex_unlock(&buffer->lock);

	return 0;
}

static void exp_sys_heap_detach(struct dma_buf *dmabuf, struct dma_buf_attachment *attachment) {
	struct exp_sys_heap_buffer *buffer = dmabuf->priv;
	struct exp_sys_heap_attachment *a = attachment->priv;

	mutex_lock(&buffer->lock);
	list_del(&a->list);
	mutex_unlock(&buffer->lock);

	sg_free_table(a->table);
	kfree(a->table);
	kfree(a);
}

static void exp_sys_heap_dma_buf_release(struct dma_buf *dmabuf) {
	struct exp_sys_heap_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct scatterlist *sg;
	int i;

	table = &buffer->sg_table;
	for_each_sg(table->sgl, sg, table->nents, i) {
		struct page *page = sg_page(sg);

		__free_pages(page, compound_order(page));
	}
	sg_free_table(table);
	kfree(buffer);
}

static struct sg_table * exp_sys_heap_map_dma_buf(struct dma_buf_attachment *attachment,
	enum dma_data_direction direction) {
	struct exp_sys_heap_attachment *a = attachment->priv;
	struct sg_table *table = a->table;
	int ret;

	ret = dma_map_sgtable(attachment->dev, table, direction, 0);
	if (ret) {
		pr_err("dma_map_sgtable() failed, err=%d\n", ret);

		return ERR_PTR(ret);
	}

	a->mapped = true;
	return table;
}

static void exp_sys_heap_unmap_dma_buf(struct dma_buf_attachment * attachment,
	struct sg_table * table,
	enum dma_data_direction direction) {
	struct exp_sys_heap_attachment *a = attachment->priv;

	a->mapped = false;
	dma_unmap_sgtable(attachment->dev, table, direction, 0);
}

static int exp_sys_heap_begin_cpu_access(struct dma_buf *dmabuf,
						enum dma_data_direction direction)
{
	struct exp_sys_heap_buffer *buffer = dmabuf->priv;
	struct exp_sys_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		invalidate_kernel_vmap_range(buffer->vaddr, buffer->len);

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sgtable_for_cpu(a->dev, a->table, direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static int exp_sys_heap_end_cpu_access(struct dma_buf *dmabuf,
					      enum dma_data_direction direction)
{
	struct exp_sys_heap_buffer *buffer = dmabuf->priv;
	struct exp_sys_heap_attachment *a;

	mutex_lock(&buffer->lock);

	if (buffer->vmap_cnt)
		flush_kernel_vmap_range(buffer->vaddr, buffer->len);

	list_for_each_entry(a, &buffer->attachments, list) {
		if (!a->mapped)
			continue;
		dma_sync_sgtable_for_device(a->dev, a->table, direction);
	}
	mutex_unlock(&buffer->lock);

	return 0;
}

static void *exp_sys_heap_do_vmap(struct exp_sys_heap_buffer *buffer)
{
	struct sg_table *table = &buffer->sg_table;
	int npages = PAGE_ALIGN(buffer->len) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct sg_page_iter piter;
	void *vaddr;

	if (!pages)
		return ERR_PTR(-ENOMEM);

	for_each_sgtable_page(table, &piter, 0) {
		WARN_ON(tmp - pages >= npages);
		*tmp++ = sg_page_iter_page(&piter);
	}

	vaddr = vmap(pages, npages, VM_MAP, PAGE_KERNEL);
	vfree(pages);

	if (!vaddr)
		return ERR_PTR(-ENOMEM);

	return vaddr;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,10,120)
static void * exp_sys_heap_vmap(struct dma_buf *dmabuf)
#else
static int exp_sys_heap_vmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
#endif
{
	struct exp_sys_heap_buffer *buffer = dmabuf->priv;
	void *vaddr;
	int ret = 0;

	mutex_lock(&buffer->lock);
	if (buffer->vmap_cnt) {
		buffer->vmap_cnt++;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,10,120)
		vaddr = buffer->vaddr;
#else
		dma_buf_map_set_vaddr(map, buffer->vaddr);

		ret = 0;
#endif
		goto out;
	}

	vaddr = exp_sys_heap_do_vmap(buffer);
	if (IS_ERR(vaddr)) {
		ret = PTR_ERR(vaddr);
		goto out;
	}

	buffer->vaddr = vaddr;
	buffer->vmap_cnt++;

#if LINUX_VERSION_CODE > KERNEL_VERSION(5,10,120)
	dma_buf_map_set_vaddr(map, buffer->vaddr);

	ret = 0;
#endif

out:
	mutex_unlock(&buffer->lock);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,10,120)
	return vaddr;
#else
	return ret;
#endif
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,10,120)
static void exp_sys_heap_vunmap(struct dma_buf *dmabuf, void * vaddr)
#else
static void exp_sys_heap_vunmap(struct dma_buf *dmabuf, struct dma_buf_map *map)
#endif
{
	struct exp_sys_heap_buffer *buffer = dmabuf->priv;

	mutex_lock(&buffer->lock);
	if (!--buffer->vmap_cnt) {
		vunmap(buffer->vaddr);
		buffer->vaddr = NULL;
	}
	mutex_unlock(&buffer->lock);

#if LINUX_VERSION_CODE > KERNEL_VERSION(5,10,120)
	dma_buf_map_clear(map);
#endif
}

static int exp_sys_heap_mmap(struct dma_buf *dmabuf, struct vm_area_struct *vma)
{
	struct exp_sys_heap_buffer *buffer = dmabuf->priv;
	struct sg_table *table = &buffer->sg_table;
	unsigned long addr = vma->vm_start;
	struct sg_page_iter piter;
	int ret;

	for_each_sgtable_page(table, &piter, vma->vm_pgoff) {
		struct page *page = sg_page_iter_page(&piter);

		ret = remap_pfn_range(vma, addr, page_to_pfn(page), PAGE_SIZE,
				      vma->vm_page_prot);
		if (ret)
			return ret;
		addr += PAGE_SIZE;
		if (addr >= vma->vm_end)
			return 0;
	}
	return 0;
}

static const struct dma_buf_ops exp_sys_heap_buf_ops = {
	.attach = exp_sys_heap_attach,
	.detach = exp_sys_heap_detach,
	.map_dma_buf = exp_sys_heap_map_dma_buf,
	.unmap_dma_buf = exp_sys_heap_unmap_dma_buf,
	.begin_cpu_access = exp_sys_heap_begin_cpu_access,
	.end_cpu_access = exp_sys_heap_end_cpu_access,
	.mmap = exp_sys_heap_mmap,
	.vmap = exp_sys_heap_vmap,
	.vunmap = exp_sys_heap_vunmap,
	.release = exp_sys_heap_dma_buf_release,
};

static struct page *alloc_largest_available(unsigned long size,
					    unsigned int max_order)
{
	struct page *page;
	int i;

	for (i = 0; i < NUM_ORDERS; i++) {
		if (size <  (PAGE_SIZE << orders[i]))
			continue;
		if (max_order < orders[i])
			continue;

		page = alloc_pages(order_flags[i], orders[i]);
		if (!page)
			continue;
		return page;
	}
	return NULL;
}

static struct dma_buf *exp_sys_heap_allocate(unsigned long len, unsigned long fd_flags)
{
	struct exp_sys_heap_buffer *buffer;
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	unsigned long size_remaining = len;
	unsigned int max_order = orders[0];
	struct dma_buf *dmabuf;
	struct sg_table *table;
	struct scatterlist *sg;
	struct list_head pages;
	struct page *page, *tmp_page;
	int i, ret = -ENOMEM;

	buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&buffer->attachments);
	mutex_init(&buffer->lock);
	buffer->len = len;

	INIT_LIST_HEAD(&pages);
	i = 0;
	while (size_remaining > 0) {
		/*
		 * Avoid trying to allocate memory if the process
		 * has been killed by SIGKILL
		 */
		if (fatal_signal_pending(current)) {
			ret = -EINTR;
			goto free_buffer;
		}

		page = alloc_largest_available(size_remaining, max_order);
		if (!page)
			goto free_buffer;

		list_add_tail(&page->lru, &pages);
		size_remaining -= page_size(page);
		max_order = compound_order(page);
		i++;
	}

	table = &buffer->sg_table;
	if (sg_alloc_table(table, i, GFP_KERNEL))
		goto free_buffer;

	sg = table->sgl;
	list_for_each_entry_safe(page, tmp_page, &pages, lru) {
		sg_set_page(sg, page, page_size(page), 0);
		sg = sg_next(sg);
		list_del(&page->lru);
	}

	/* create the dmabuf */
	exp_info.exp_name = "qdmabuf-sys-heap";
	exp_info.ops = &exp_sys_heap_buf_ops;
	exp_info.size = buffer->len;
	exp_info.flags = fd_flags;
	exp_info.priv = buffer;
	dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(dmabuf)) {
		ret = PTR_ERR(dmabuf);
		goto free_pages;
	}
	return dmabuf;

free_pages:
	for_each_sgtable_sg(table, sg, i) {
		struct page *p = sg_page(sg);

		__free_pages(p, compound_order(p));
	}
	sg_free_table(table);
free_buffer:
	list_for_each_entry_safe(page, tmp_page, &pages, lru)
		__free_pages(page, compound_order(page));
	kfree(buffer);

	return ERR_PTR(ret);
}

int qdmabuf_dmabuf_alloc_sys_heap(struct device* device, int len, int fd_flags, int dma_dir) {
	struct dma_buf *dmabuf;
	int ret;

	dmabuf = exp_sys_heap_allocate(len, fd_flags);
	if (IS_ERR(dmabuf)) {
		pr_err("exp_sys_heap_allocate() failed, dmabuf=%p\n", dmabuf);
		return PTR_ERR(dmabuf);
	}

	ret = dma_buf_fd(dmabuf, fd_flags);
	if (ret < 0) {
		pr_err("dma_buf_fd() failed, err=%d\n", ret);
		dma_buf_put(dmabuf);

		return ret;
	}

	return ret;
}
#endif