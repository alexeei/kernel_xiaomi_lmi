// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017-2018 HUAWEI, Inc.
 *             https://www.huawei.com/
 * Copyright (C) 2021, Alibaba Cloud
 */
#include "internal.h"
#include <linux/prefetch.h>

#include <linux/blkdev.h>
#include <linux/uio.h>

#include <trace/events/erofs.h>

void erofs_unmap_metabuf(struct erofs_buf *buf)
{
	if (buf->kmap_type == EROFS_KMAP)
		kunmap(buf->page);
	else if (buf->kmap_type == EROFS_KMAP_ATOMIC)
		kunmap_atomic(buf->base);
	buf->base = NULL;
	buf->kmap_type = EROFS_NO_KMAP;
}

void erofs_put_metabuf(struct erofs_buf *buf)
{
	if (!buf->page)
		return;
	erofs_unmap_metabuf(buf);
	put_page(buf->page);
	buf->page = NULL;
}

void *erofs_bread(struct erofs_buf *buf, struct inode *inode,
		  erofs_blk_t blkaddr, enum erofs_kmap_type type)
{
	struct address_space *const mapping = inode->i_mapping;
	erofs_off_t offset = blknr_to_addr(blkaddr);
	pgoff_t index = offset >> PAGE_SHIFT;
	struct page *page = buf->page;

	if (!page || page->index != index) {
		erofs_put_metabuf(buf);
		page = read_cache_page_gfp(mapping, index,
				mapping_gfp_constraint(mapping, ~__GFP_FS));
		if (IS_ERR(page))
			return page;
		/* should already be PageUptodate, no need to lock page */
		buf->page = page;
	}
	if (buf->kmap_type == EROFS_NO_KMAP) {
		if (type == EROFS_KMAP)
			buf->base = kmap(page);
		else if (type == EROFS_KMAP_ATOMIC)
			buf->base = kmap_atomic(page);
		buf->kmap_type = type;
	} else if (buf->kmap_type != type) {
		DBG_BUGON(1);
		return ERR_PTR(-EFAULT);
	}
	if (type == EROFS_NO_KMAP)
		return NULL;
	return buf->base + (offset & ~PAGE_MASK);
}

void *erofs_read_metabuf(struct erofs_buf *buf, struct super_block *sb,
			 erofs_blk_t blkaddr, enum erofs_kmap_type type)
{
	return erofs_bread(buf, sb->s_bdev->bd_inode, blkaddr, type);
}

static int erofs_map_blocks_flatmode(struct inode *inode,
				     struct erofs_map_blocks *map,
				     int flags)
{
	erofs_blk_t nblocks, lastblk;
	u64 offset = map->m_la;
	struct erofs_inode *vi = EROFS_I(inode);
	bool tailendpacking = (vi->datalayout == EROFS_INODE_FLAT_INLINE);

	nblocks = DIV_ROUND_UP(inode->i_size, EROFS_BLKSIZ);
	lastblk = nblocks - tailendpacking;

	/* there is no hole in flatmode */
	map->m_flags = EROFS_MAP_MAPPED;
	if (offset < blknr_to_addr(lastblk)) {
		map->m_pa = blknr_to_addr(vi->raw_blkaddr) + map->m_la;
		map->m_plen = blknr_to_addr(lastblk) - offset;
	} else if (tailendpacking) {
		/* 2 - inode inline B: inode, [xattrs], inline last blk... */
		struct erofs_sb_info *sbi = EROFS_SB(inode->i_sb);

		map->m_pa = iloc(sbi, vi->nid) + vi->inode_isize +
			vi->xattr_isize + erofs_blkoff(map->m_la);
		map->m_plen = inode->i_size - offset;

		/* inline data should be located in the same meta block */
		if (erofs_blkoff(map->m_pa) + map->m_plen > EROFS_BLKSIZ) {
			erofs_err(inode->i_sb,
				  "inline data cross block boundary @ nid %llu",
				  vi->nid);
			DBG_BUGON(1);
			return -EFSCORRUPTED;
		}
		map->m_flags |= EROFS_MAP_META;
	} else {
		erofs_err(inode->i_sb,
			  "internal error @ nid: %llu (size %llu), m_la 0x%llx",
			  vi->nid, inode->i_size, map->m_la);
		DBG_BUGON(1);
		return -EIO;
	}
	return 0;
}


static inline struct bio *erofs_read_raw_page(struct bio *bio,
					      struct address_space *mapping,
					      struct page *page,
					      erofs_off_t *last_block,
					      unsigned int nblocks,
					      bool ra)

{
	struct erofs_dev_context *devs = EROFS_SB(sb)->devs;
	struct erofs_device_info *dif;
	int id;

	/* primary device by default */
	map->m_bdev = sb->s_bdev;


	if (PageUptodate(page)) {
		err = 0;
		goto has_updated;
	}

	/* note that for readpage case, bio also equals to NULL */
	if (bio &&
	    /* not continuous */
	    *last_block + 1 != current_block) {
submit_bio_retry:
		submit_bio(bio);
		bio = NULL;
	}

	if (!bio) {
		struct erofs_map_blocks map = {
			.m_la = blknr_to_addr(current_block),
		};
		erofs_blk_t blknr;
		unsigned int blkoff;

		err = erofs_map_blocks_flatmode(inode, &map, EROFS_GET_BLOCKS_RAW);
		if (err)
			goto err_out;

		/* zero out the holed page */
		if (!(map.m_flags & EROFS_MAP_MAPPED)) {
			zero_user_segment(page, 0, PAGE_SIZE);
			SetPageUptodate(page);

			/* imply err = 0, see erofs_map_blocks */
			goto has_updated;
		}
		map->m_bdev = dif->bdev;
		up_read(&devs->rwsem);
	} else if (devs->extra_devices) {
		down_read(&devs->rwsem);
		idr_for_each_entry(&devs->tree, dif, id) {
			erofs_off_t startoff, length;

			if (!dif->mapped_blkaddr)
				continue;
			startoff = blknr_to_addr(dif->mapped_blkaddr);
			length = blknr_to_addr(dif->blocks);

			if (map->m_pa >= startoff &&
			    map->m_pa < startoff + length) {
				map->m_pa -= startoff;
				map->m_bdev = dif->bdev;
				break;
			}
		}
		up_read(&devs->rwsem);
	}
	return 0;
}

static int erofs_iomap_begin(struct inode *inode, loff_t offset, loff_t length,
		unsigned int flags, struct iomap *iomap)
{
	int ret;
	struct erofs_map_blocks map;
	struct erofs_map_dev mdev;

	map.m_la = offset;
	map.m_llen = length;

	ret = erofs_map_blocks(inode, &map, EROFS_GET_BLOCKS_RAW);
	if (ret < 0)
		return ret;

	mdev = (struct erofs_map_dev) {
		.m_deviceid = map.m_deviceid,
		.m_pa = map.m_pa,
	};
	ret = erofs_map_dev(inode->i_sb, &mdev);
	if (ret)
		return ret;

	iomap->bdev = mdev.m_bdev;
	iomap->offset = map.m_la;
	iomap->length = map.m_llen;
	iomap->flags = 0;
	iomap->private = NULL;

	if (!(map.m_flags & EROFS_MAP_MAPPED)) {
		iomap->type = IOMAP_HOLE;
		iomap->addr = IOMAP_NULL_ADDR;
		if (!iomap->length)
			iomap->length = length;
		return 0;
	}

	if (map.m_flags & EROFS_MAP_META) {
		void *ptr;
		struct erofs_buf buf = __EROFS_BUF_INITIALIZER;

		iomap->type = IOMAP_INLINE;
		ptr = erofs_read_metabuf(&buf, inode->i_sb,
					 erofs_blknr(mdev.m_pa), EROFS_KMAP);
		if (IS_ERR(ptr))
			return PTR_ERR(ptr);
		iomap->inline_data = ptr + erofs_blkoff(mdev.m_pa);
		iomap->private = buf.base;
	} else {
		iomap->type = IOMAP_MAPPED;
		iomap->addr = mdev.m_pa;
	}
	return 0;
}

static int erofs_iomap_end(struct inode *inode, loff_t pos, loff_t length,
		ssize_t written, unsigned int flags, struct iomap *iomap)
{
	void *ptr = iomap->private;

	if (ptr) {
		struct erofs_buf buf = {
			.page = kmap_to_page(ptr),
			.base = ptr,
			.kmap_type = EROFS_KMAP,
		};

		DBG_BUGON(iomap->type != IOMAP_INLINE);
		erofs_put_metabuf(&buf);
	} else {
		DBG_BUGON(iomap->type == IOMAP_INLINE);
	}
	return written;
}

static const struct iomap_ops erofs_iomap_ops = {
	.iomap_begin = erofs_iomap_begin,
	.iomap_end = erofs_iomap_end,
};

int erofs_fiemap(struct inode *inode, struct fiemap_extent_info *fieinfo,
		 u64 start, u64 len)
{
	if (erofs_inode_is_data_compressed(EROFS_I(inode)->datalayout)) {
#ifdef CONFIG_EROFS_FS_ZIP
		return iomap_fiemap(inode, fieinfo, start, len,
				    &z_erofs_iomap_report_ops);
#else
		return -EOPNOTSUPP;
#endif
	}
	return iomap_fiemap(inode, fieinfo, start, len, &erofs_iomap_ops);
}

/*
 * since we dont have write or truncate flows, so no inode
 * locking needs to be held at the moment.
 */
static int erofs_readpage(struct file *file, struct page *page)
{
	return iomap_readpage(page, &erofs_iomap_ops);
}

static int erofs_readpages(struct file *file,
			struct address_space *mapping,
			struct list_head *pages, unsigned nr_pages)
{
	return iomap_readpages(mapping, pages, nr_pages, &erofs_iomap_ops);
}

static sector_t erofs_bmap(struct address_space *mapping, sector_t block)
{
	return iomap_bmap(mapping, block, &erofs_iomap_ops);
}

static ssize_t erofs_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	struct inode *inode = file_inode(iocb->ki_filp);

	/* no need taking (shared) inode lock since it's a ro filesystem */
	if (!iov_iter_count(to))
		return 0;

	if (iocb->ki_flags & IOCB_DIRECT) {
		struct block_device *bdev = inode->i_sb->s_bdev;
		unsigned int blksize_mask;

		if (bdev)
			blksize_mask = bdev_logical_block_size(bdev) - 1;
		else
			blksize_mask = (1 << inode->i_blkbits) - 1;

		if ((iocb->ki_pos | iov_iter_count(to) |
		     iov_iter_alignment(to)) & blksize_mask)
			return -EINVAL;

		return iomap_dio_rw(iocb, to, &erofs_iomap_ops, NULL);
	}


	if (!erofs_map_blocks_flatmode(inode, &map, EROFS_GET_BLOCKS_RAW))
		return erofs_blknr(map.m_pa);

	return 0;

}

/* for uncompressed (aligned) files and raw access for other files */
const struct address_space_operations erofs_raw_access_aops = {
	.readpage = erofs_readpage,
	.readpages = erofs_readpages,
	.bmap = erofs_bmap,
	.direct_IO = noop_direct_IO,
};

const struct file_operations erofs_file_fops = {
	.llseek		= generic_file_llseek,
	.read_iter	= erofs_file_read_iter,
	.mmap		= generic_file_readonly_mmap,
	.splice_read	= generic_file_splice_read,
};
