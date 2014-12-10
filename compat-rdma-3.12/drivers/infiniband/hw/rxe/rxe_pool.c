/*
 * Copyright (c) 2009-2011 Mellanox Technologies Ltd. All rights reserved.
 * Copyright (c) 2009-2011 System Fabric Works, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *	   Redistribution and use in source and binary forms, with or
 *	   without modification, are permitted provided that the following
 *	   conditions are met:
 *
 *		- Redistributions of source code must retain the above
 *		  copyright notice, this list of conditions and the following
 *		  disclaimer.
 *
 *		- Redistributions in binary form must reproduce the above
 *		  copyright notice, this list of conditions and the following
 *		  disclaimer in the documentation and/or other materials
 *		  provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "rxe.h"
#include "rxe_loc.h"

/* info about object pools
   note that mr, fmr and mw share a single index space
   so that one can map an lkey to the correct type of object */
struct rxe_type_info rxe_type_info[RXE_NUM_TYPES] = {
	[RXE_TYPE_UC] = {
		.name		= "rxe_uc",
		.size		= sizeof(struct rxe_ucontext),
	},
	[RXE_TYPE_PD] = {
		.name		= "rxe_pd",
		.size		= sizeof(struct rxe_pd),
	},
	[RXE_TYPE_AH] = {
		.name		= "rxe_ah",
		.size		= sizeof(struct rxe_ah),
		.flags		= RXE_POOL_ATOMIC,
	},
	[RXE_TYPE_SRQ] = {
		.name		= "rxe_srq",
		.size		= sizeof(struct rxe_srq),
		.flags		= RXE_POOL_INDEX,
		.min_index	= RXE_MIN_SRQ_INDEX,
		.max_index	= RXE_MAX_SRQ_INDEX,
		.cleanup	= rxe_srq_cleanup,
	},
	[RXE_TYPE_QP] = {
		.name		= "rxe_qp",
		.size		= sizeof(struct rxe_qp),
		.cleanup	= rxe_qp_cleanup,
		.flags		= RXE_POOL_INDEX,
		.min_index	= RXE_MIN_QP_INDEX,
		.max_index	= RXE_MAX_QP_INDEX,
	},
	[RXE_TYPE_CQ] = {
		.name		= "rxe_cq",
		.size		= sizeof(struct rxe_cq),
		.cleanup	= rxe_cq_cleanup,
	},
	[RXE_TYPE_MR] = {
		.name		= "rxe_mr",
		.size		= sizeof(struct rxe_mem),
		.cleanup	= rxe_mem_cleanup,
		.flags		= RXE_POOL_INDEX,
		.max_index	= RXE_MAX_MR_INDEX,
		.min_index	= RXE_MIN_MR_INDEX,
	},
	[RXE_TYPE_FMR] = {
		.name		= "rxe_fmr",
		.size		= sizeof(struct rxe_mem),
		.cleanup	= rxe_mem_cleanup,
		.flags		= RXE_POOL_INDEX,
		.max_index	= RXE_MAX_FMR_INDEX,
		.min_index	= RXE_MIN_FMR_INDEX,
	},
	[RXE_TYPE_MW] = {
		.name		= "rxe_mw",
		.size		= sizeof(struct rxe_mem),
		.flags		= RXE_POOL_INDEX,
		.max_index	= RXE_MAX_MW_INDEX,
		.min_index	= RXE_MIN_MW_INDEX,
	},
	[RXE_TYPE_MC_GRP] = {
		.name		= "rxe_mc_grp",
		.size		= sizeof(struct rxe_mc_grp),
		.cleanup	= rxe_mc_cleanup,
		.flags		= RXE_POOL_KEY,
		.key_offset	= offsetof(struct rxe_mc_grp, mgid),
		.key_size	= sizeof(union ib_gid),
	},
	[RXE_TYPE_MC_ELEM] = {
		.name		= "rxe_mc_elem",
		.size		= sizeof(struct rxe_mc_elem),
		.flags		= RXE_POOL_ATOMIC,
	},
};

static inline char *pool_name(struct rxe_pool *pool)
{
	return rxe_type_info[pool->type].name + 4;
}

static inline struct kmem_cache *pool_cache(struct rxe_pool *pool)
{
	return rxe_type_info[pool->type].cache;
}

static inline enum rxe_elem_type rxe_type(void *arg)
{
	struct rxe_pool_entry *elem = arg;
	return elem->pool->type;
}

int __init rxe_cache_init(void)
{
	int err;
	int i;
	size_t size;
	struct rxe_type_info *type;

	for (i = 0; i < RXE_NUM_TYPES; i++) {
		type = &rxe_type_info[i];
		size = (type->size + RXE_POOL_ALIGN - 1) &
				~(RXE_POOL_ALIGN - 1);
		type->cache = kmem_cache_create(type->name, size,
				RXE_POOL_ALIGN,
				RXE_POOL_CACHE_FLAGS, NULL);
		if (!type->cache) {
			pr_info("Unable to init kmem cache for %s\n",
				type->name);
			err = -ENOMEM;
			goto err1;
		}
	}

	return 0;

err1:
	while (--i >= 0) {
		kmem_cache_destroy(type->cache);
		type->cache = NULL;
	}

	return err;
}

void __exit rxe_cache_exit(void)
{
	int i;
	struct rxe_type_info *type;

	for (i = 0; i < RXE_NUM_TYPES; i++) {
		type = &rxe_type_info[i];
		kmem_cache_destroy(type->cache);
		type->cache = NULL;
	}
}

static int rxe_pool_init_index(struct rxe_pool *pool, u32 max, u32 min)
{
	int err = 0;
	size_t size;

	if ((max - min + 1) < pool->max_elem) {
		pr_warn("not enough indices for max_elem\n");
		err = -EINVAL;
		goto out;
	}

	pool->max_index = max;
	pool->min_index = min;

	size = BITS_TO_LONGS(max - min + 1) * sizeof(long);
	pool->table = kmalloc(size, GFP_KERNEL);
	if (!pool->table) {
		pr_warn("no memory for bit table\n");
		err = -ENOMEM;
		goto out;
	}

	pool->table_size = size;
	bitmap_zero(pool->table, max - min + 1);

out:
	return err;
}

int rxe_pool_init(
	struct rxe_dev		*rxe,
	struct rxe_pool		*pool,
	enum rxe_elem_type	type,
	unsigned		max_elem)
{
	int			err = 0;
	size_t			size = rxe_type_info[type].size;

	memset(pool, 0, sizeof(*pool));

	pool->rxe		= rxe;
	pool->type		= type;
	pool->max_elem		= max_elem;
	pool->elem_size		= (size + RXE_POOL_ALIGN - 1) &
					~(RXE_POOL_ALIGN - 1);
	pool->flags		= rxe_type_info[type].flags;
	pool->tree		= RB_ROOT;
	pool->cleanup		= rxe_type_info[type].cleanup;

	atomic_set(&pool->num_elem, 0);

	kref_init(&pool->ref_cnt);

	spin_lock_init(&pool->pool_lock);

	if (rxe_type_info[type].flags & RXE_POOL_INDEX) {
		err = rxe_pool_init_index(pool,
			rxe_type_info[type].max_index,
			rxe_type_info[type].min_index);
		if (err)
			goto out;
	}

	if (rxe_type_info[type].flags & RXE_POOL_KEY) {
		pool->key_offset = rxe_type_info[type].key_offset;
		pool->key_size = rxe_type_info[type].key_size;
	}

	pool->state = rxe_pool_valid;

out:
	return err;
}

static void rxe_pool_release(struct kref *kref)
{
	struct rxe_pool *pool = container_of(kref, struct rxe_pool, ref_cnt);
#if 0
	unsigned long flags;
#endif

#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif
	pool->state = rxe_pool_invalid;
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif

	kfree(pool->table);

	return;
}

int rxe_pool_cleanup(struct rxe_pool *pool)
{
	int num_elem;
#if 0
	unsigned long flags;
#endif

#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif
	pool->state = rxe_pool_invalid;
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif

	num_elem = atomic_read(&pool->num_elem);
	if (num_elem > 0)
		pr_warn("%s pool destroyed with %d unfree'd elem\n",
			 pool_name(pool), num_elem);

	kref_put(&pool->ref_cnt, rxe_pool_release);

	return 0;
}

static u32 alloc_index(struct rxe_pool *pool)
{
	u32 index;
	u32 range = pool->max_index - pool->min_index + 1;

	index = find_next_zero_bit(pool->table, range, pool->last);
	if (index >= range)
		index = find_first_zero_bit(pool->table, range);

	set_bit(index, pool->table);
	pool->last = index;
	return index + pool->min_index;
}

static void insert_index(struct rxe_pool *pool, struct rxe_pool_entry *new)
{
	struct rb_node **link = &pool->tree.rb_node;
	struct rb_node *parent = NULL;
	struct rxe_pool_entry *elem;

	while (*link) {
		parent = *link;
		elem = rb_entry(parent, struct rxe_pool_entry, node);

		if (elem->index == new->index)
			goto out;

		if (elem->index > new->index)
			link = &(*link)->rb_left;
		else
			link = &(*link)->rb_right;
	}

	rb_link_node(&new->node, parent, link);
	rb_insert_color(&new->node, &pool->tree);
out:
	return;
}

static void insert_key(struct rxe_pool *pool, struct rxe_pool_entry *new)
{
	struct rb_node **link = &pool->tree.rb_node;
	struct rb_node *parent = NULL;
	struct rxe_pool_entry *elem;
	int cmp;

	while (*link) {
		parent = *link;
		elem = rb_entry(parent, struct rxe_pool_entry, node);

		cmp = memcmp((u8 *)elem + pool->key_offset,
			(u8 *)new + pool->key_offset, pool->key_size);

		if (cmp == 0)
			goto out;

		if (cmp > 0)
			link = &(*link)->rb_left;
		else
			link = &(*link)->rb_right;
	}

	rb_link_node(&new->node, parent, link);
	rb_insert_color(&new->node, &pool->tree);
out:
	return;
}

void rxe_add_key(void *arg, void *key)
{
	struct rxe_pool_entry *elem = arg;
	struct rxe_pool *pool = elem->pool;
#if 0
	unsigned long flags;
#endif

#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif
	memcpy((u8 *)elem + pool->key_offset, key, pool->key_size);
	insert_key(pool, elem);
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif
	return;
}

void rxe_drop_key(void *arg)
{
	struct rxe_pool_entry *elem = arg;
	struct rxe_pool *pool = elem->pool;
#if 0
	unsigned long flags;
#endif

#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif
	rb_erase(&elem->node, &pool->tree);
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif
	return;
}

void rxe_add_index(void *arg)
{
	struct rxe_pool_entry *elem = arg;
	struct rxe_pool *pool = elem->pool;
#if 0
	unsigned long flags;
#endif

#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif
	elem->index = alloc_index(pool);
	insert_index(pool, elem);
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif
	return;
}

void rxe_drop_index(void *arg)
{
	struct rxe_pool_entry *elem = arg;
	struct rxe_pool *pool = elem->pool;
#if 0
	unsigned long flags;
#endif

#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif
	clear_bit(elem->index - pool->min_index, pool->table);
	rb_erase(&elem->node, &pool->tree);
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif
	return;
}

void *rxe_alloc(struct rxe_pool *pool)
{
	struct rxe_pool_entry *elem;
#if 0
	unsigned long flags;
#endif

	if (!(pool->flags & RXE_POOL_ATOMIC)
	    && (in_irq() || irqs_disabled())) {
		pr_warn("pool alloc %s in context %d/%d\n",
		       pool_name(pool), (int)in_irq(),
		       (int)irqs_disabled());
	}

#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif
	if (pool->state != rxe_pool_valid) {
#if 1
		spin_unlock_bh(&pool->pool_lock);
#else
		spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif
		return NULL;
	}
	kref_get(&pool->ref_cnt);
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif

	kref_get(&pool->rxe->ref_cnt);

	if (atomic_inc_return(&pool->num_elem) > pool->max_elem) {
		atomic_dec(&pool->num_elem);
		kref_put(&pool->rxe->ref_cnt, rxe_release);
		kref_put(&pool->ref_cnt, rxe_pool_release);
		return NULL;
	}

	elem = kmem_cache_zalloc(pool_cache(pool),
				 (pool->flags & RXE_POOL_ATOMIC) ?
				 GFP_ATOMIC : GFP_KERNEL);

	elem->pool = pool;
	kref_init(&elem->ref_cnt);

	return elem;
}

void rxe_elem_release(struct kref *kref)
{
	struct rxe_pool_entry *elem =
		container_of(kref, struct rxe_pool_entry, ref_cnt);
	struct rxe_pool *pool = elem->pool;

	if (pool->cleanup)
		pool->cleanup(elem);

	kmem_cache_free(pool_cache(pool), elem);
	atomic_dec(&pool->num_elem);
	kref_put(&pool->rxe->ref_cnt, rxe_release);
	kref_put(&pool->ref_cnt, rxe_pool_release);
}

void *rxe_pool_get_index(struct rxe_pool *pool, u32 index)
{
	struct rb_node *node = NULL;
	struct rxe_pool_entry *elem = NULL;
#if 0
	unsigned long flags;
#endif
#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif

	if (pool->state != rxe_pool_valid)
		goto out;

	node = pool->tree.rb_node;

	while (node) {
		elem = rb_entry(node, struct rxe_pool_entry, node);

		if (elem->index > index)
			node = node->rb_left;
		else if (elem->index < index)
			node = node->rb_right;
		else
			break;
	}

	if (node)
		kref_get(&elem->ref_cnt);

out:
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif
	return node ? (void *)elem : NULL;
}

void *rxe_pool_get_key(struct rxe_pool *pool, void *key)
{
	struct rb_node *node = NULL;
	struct rxe_pool_entry *elem = NULL;
	int cmp;
#if 0
	unsigned long flags;
#endif

#if 1
	spin_lock_bh(&pool->pool_lock);
#else
	spin_lock_irqsave(&pool->pool_lock, flags);
#endif

	if (pool->state != rxe_pool_valid)
		goto out;

	node = pool->tree.rb_node;

	while (node) {
		elem = rb_entry(node, struct rxe_pool_entry, node);

		cmp = memcmp((u8 *)elem + pool->key_offset,
			     key, pool->key_size);

		if (cmp > 0)
			node = node->rb_left;
		else if (cmp < 0)
			node = node->rb_right;
		else
			break;
	}

	if (node)
		kref_get(&elem->ref_cnt);

out:
#if 1
	spin_unlock_bh(&pool->pool_lock);
#else
	spin_unlock_irqrestore(&pool->pool_lock, flags);
#endif
	return node ? ((void *)elem) : NULL;
}
