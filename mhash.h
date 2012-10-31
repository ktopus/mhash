/*
 * Copyright (C) 2011 Mail.RU
 * Copyright (C) 2011 Yuriy Vostrikov
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
e * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* The MIT License

   Copyright (c) 2008, by Attractive Chaos <attractivechaos@aol.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#ifndef MH_INCREMENTAL_RESIZE
#define MH_INCREMENTAL_RESIZE 1
#endif

#ifdef MH_STATIC
#define MH_SOURCE
#define MH_DECL static inline
#endif

#ifndef MH_DECL
#define MH_DECL
#endif

#ifndef MH_HELPER_MACRO
#define MH_HELPER_MACRO
#define mh_cat(a, b) mh##a##_##b
#define mh_ecat(a, b) mh_cat(a, b)
#define _mh(x) mh_ecat(mh_name, x)
#define mh_unlikely(x)  __builtin_expect((x),0)

#define mh_exist(h, i)		({ h->bitmap[i >> 4] & (1 << (i % 16)); })
#define mh_dirty(h, i)		({ h->bitmap[i >> 4] & (1 << (i % 16 + 16)); })

#define mh_setfree(h, i)	({ h->bitmap[i >> 4] &= ~(1 << (i % 16)); })
#define mh_setexist(h, i)	({ h->bitmap[i >> 4] |= (1 << (i % 16)); })
#define mh_setdirty(h, i)	({ h->bitmap[i >> 4] |= (1 << (i % 16 + 16)); })

#define mh_slot(h, i)		({ ((h)->nodes + (size_t)(h)->node_size * (i)); })
#define mh_node_key(node) 	*(mh_key_t *)((void *)node + sizeof(mh_val_t))

#define mh_size(h)		({ (h)->size; 		})
#define mh_end(h)		({ (h)->n_buckets;	})
#endif

#ifndef __ac_HASH_PRIME_SIZE
#define __ac_HASH_PRIME_SIZE 31
static const uint32_t __ac_prime_list[__ac_HASH_PRIME_SIZE] = {
	3ul,		11ul,		23ul,		53ul,
	97ul,		193ul,		389ul,		769ul,
	1543ul,		3079ul,		6151ul,		12289ul,
	24593ul,	49157ul,	98317ul,	196613ul,
	393241ul,	786433ul,	1572869ul,	3145739ul,
	6291469ul,	12582917ul,	25165843ul,	50331653ul,
	100663319ul,	201326611ul,	402653189ul,	805306457ul,
	1610612741ul,	3221225473ul,	4294967291ul
};
#endif

#ifndef MH_HASH_T
#define MH_HASH_T
struct index_node;
struct mhash_t {
	void *nodes;
	uint32_t node_size;
	uint32_t *bitmap;
	uint32_t n_buckets, n_occupied, size, upper_bound;
	uint32_t prime;

	uint32_t resize_cnt;
	uint32_t resizing, batch;
	struct mhash_t *shadow;
	void *(*realloc)(void *, size_t);
};
#endif


#ifndef mh_node_size
#define mh_node_size (sizeof(mh_val_t) + sizeof(mh_key_t))
#endif

/* public api */
MH_DECL struct mhash_t * _mh(init)(void *(*custom_realloc)(void *, size_t));
static inline mh_key_t _mh(key)(struct mhash_t *h, uint32_t i);
static inline mh_val_t _mh(value)(struct mhash_t *h, uint32_t i);
static inline uint32_t _mh(get)(struct mhash_t *h, mh_key_t key);
static inline uint32_t _mh(put)(struct mhash_t *h, mh_key_t key, mh_val_t val, int *ret);
static inline uint32_t _mh(put_node)(struct mhash_t *h, struct index_node *node);
static inline uint32_t _mh(get_node)(struct mhash_t *h, struct index_node *node);
static inline uint32_t _mh(get_slot)(struct mhash_t *h, mh_key_t key);
static inline uint32_t _mh(put_slot)(struct mhash_t *h, mh_key_t key);
static inline void _mh(del)(struct mhash_t *h, uint32_t x);
MH_DECL void mh_clear(struct mhash_t *h);
MH_DECL size_t mh_bytes(struct mhash_t *h);
MH_DECL void mh_destroy(struct mhash_t *h);

/* internal api */
MH_DECL void _mh(resize)(struct mhash_t *h);
MH_DECL void _mh(start_resize)(struct mhash_t *h, uint32_t buckets, uint32_t batch);
MH_DECL void _mh(put_resize)(struct mhash_t *h, mh_key_t key, mh_val_t val);
MH_DECL void _mh(put_node_resize)(struct mhash_t *h, struct index_node *node);
MH_DECL void _mh(del_resize)(struct mhash_t *h, uint32_t x);
MH_DECL void *mh_malloc(struct mhash_t *h, size_t size);
MH_DECL void *mh_calloc(struct mhash_t *h, size_t nmemb, size_t size);
MH_DECL void mh_free(struct mhash_t *h, void *ptr);
#ifdef MH_DEBUG
MH_DECL void _mh(dump)(struct mhash_t *h);
#endif

static inline mh_key_t
_mh(key)(struct mhash_t *h, uint32_t i)
{
	return mh_node_key(mh_slot(h, i));
}

static inline mh_val_t
_mh(value)(struct mhash_t *h, uint32_t i)
{
	return *(mh_val_t *)mh_slot(h, i);
}

static inline uint32_t
_mh(get_slot)(struct mhash_t *h, mh_key_t key)
{
	uint32_t inc, k, i;
	k = mh_hash(key);
	i = k % h->n_buckets;
	inc = 1 + k % (h->n_buckets - 1);
	for (;;) {
		if ((mh_exist(h, i) && mh_eq(mh_slot(h, i), key)))
			return i;

		if (!mh_dirty(h, i))
			return h->n_buckets;

		i += inc;
		if (i >= h->n_buckets)
			i -= h->n_buckets;
	}
}

#if 0
static inline uint32_t
_mh(put_slot)(struct mhash_t *h, mh_key_t key)
{
	uint32_t inc, k, i, p = h->n_buckets;
	k = mh_hash(key);
	i = k % h->n_buckets;
	inc = 1 + k % (h->n_buckets - 1);
	for (;;) {
		if (mh_exist(h, i)) {
			if (mh_eq(mh_slot(h, i), key))
				return i;
			if (p == h->n_buckets)
				mh_setdirty(h, i);
		} else {
			if (p == h->n_buckets)
				p = i;
			if (!mh_dirty(h, i))
				return p;
		}

		i += inc;
		if (i >= h->n_buckets)
			i -= h->n_buckets;
	}
}
#endif

/* Faster variant of above loop */
static inline uint32_t
_mh(put_slot)(struct mhash_t *h, mh_key_t key)
{
	uint32_t inc, k, i, p = h->n_buckets;
	void *loop = &&marking_loop;
	k = mh_hash(key);
	i = k % h->n_buckets;
	inc = 1 + k % (h->n_buckets - 1);
marking_loop:
	if (mh_exist(h, i)) {
		if (mh_eq(mh_slot(h, i), key))
			return i;

		mh_setdirty(h, i);
		goto next_slot;
	} else {
		p = i;
		loop = &&nonmarking_loop;
		goto continue_nonmarking;
	}

nonmarking_loop:
	if (mh_exist(h, i)) {
		if (mh_eq(mh_slot(h, i), key))
			return i;
	} else {
	continue_nonmarking:
		if (!mh_dirty(h, i))
			return p;
	}

next_slot:
	i += inc;
	if (i >= h->n_buckets)
		i -= h->n_buckets;
	goto *loop;
}


static inline uint32_t
_mh(get)(struct mhash_t *h, mh_key_t key)
{
	uint32_t i = _mh(get_slot)(h, key);
	if (!mh_exist(h, i))
		return i = h->n_buckets;
#ifdef MH_PARANOIA
	assert(mh_eq(mh_slot(h, i), key));
#endif
	return i;
}

static inline uint32_t
_mh(get_node)(struct mhash_t *h, struct index_node *node)
{
	uint32_t i = _mh(get)(h, mh_node_key(node));
#ifdef MH_PARANOIA
	assert(i == h->n_buckets || mh_eq(mh_slot(h, i), mh_node_key(node)));
	assert(i <= h->n_buckets);
#endif
	return i;
}

static inline uint32_t
_mh(put)(struct mhash_t *h, mh_key_t key, mh_val_t val, int *ret)
{
#if MH_INCREMENTAL_RESIZE
	if (mh_unlikely(h->n_occupied >= h->upper_bound || h->resizing > 0))
		_mh(put_resize)(h, key, val);
#else
	if (mh_unlikely(h->n_occupied >= h->upper_bound))
		_mh(start_resize)(h, 0, -1);
#endif
	uint32_t x = _mh(put_slot)(h, key);
	int found = !mh_exist(h, x);
	if (ret)
		*ret = found;

	void *node = mh_slot(h, x);

	memcpy(node, &val, sizeof(val));

	if (found) {
		mh_setexist(h, x);
		h->size++;
		if (!mh_dirty(h, x))
			h->n_occupied++;

		memcpy(node + sizeof(val), &key, sizeof(key));
	}

	return x;
}

static inline uint32_t
_mh(put_node)(struct mhash_t *h, struct index_node *node)
{
	mh_key_t key = mh_node_key(node);
#if MH_INCREMENTAL_RESIZE
	if (mh_unlikely(h->n_occupied >= h->upper_bound || h->resizing > 0))
		_mh(put_node_resize)(h, node);
#else
	if (mh_unlikely(h->n_occupied >= h->upper_bound))
		_mh(start_resize)(h, 0, -1);
#endif
	uint32_t x = _mh(put_slot)(h, key);

	if (!mh_exist(h, x)) {
		mh_setexist(h, x);
		h->size++;
		if (!mh_dirty(h, x))
			h->n_occupied++;
	}
	memcpy(mh_slot(h, x), node, mh_node_size);
	return x;
}

static inline void
_mh(del)(struct mhash_t *h, uint32_t x)
{
	if (x != h->n_buckets && mh_exist(h, x)) {
		mh_setfree(h, x);
		h->size--;
		if (!mh_dirty(h, x))
			h->n_occupied--;
#if MH_INCREMENTAL_RESIZE
		if (mh_unlikely(h->resizing))
			_mh(del_resize)(h, x);
#endif
	}
}

#if defined(MH_SOURCE)
MH_DECL void
_mh(put_resize)(struct mhash_t *h, mh_key_t key, mh_val_t val)
{
	if (h->resizing > 0)
		_mh(resize)(h);
	else
		_mh(start_resize)(h, 0, 0);
	if (h->resizing)
		_mh(put)(h->shadow, key, val, NULL);
}

MH_DECL void
_mh(put_node_resize)(struct mhash_t *h, struct index_node *node)
{
	if (h->resizing > 0)
		_mh(resize)(h);
	else
		_mh(start_resize)(h, 0, 0);
	if (h->resizing)
		_mh(put_node)(h->shadow, node);
}

MH_DECL void
_mh(del_resize)(struct mhash_t *h, uint32_t x)
{
	struct mhash_t *s = h->shadow;
	uint32_t y = _mh(get_slot)(s, mh_node_key(mh_slot(h, x)));
	_mh(del)(s, y);
	_mh(resize)(h);
}

MH_DECL struct mhash_t *
_mh(init)(void *(*custom_realloc)(void *, size_t))
{
	custom_realloc = custom_realloc ?: realloc;
	struct mhash_t *h = custom_realloc(NULL, sizeof(*h));
	memset(h, 0, sizeof(*h));
	h->realloc = custom_realloc;
	h->node_size = mh_node_size;
	h->shadow = mh_calloc(h, 1, sizeof(*h));
	h->n_buckets = 3;
	h->nodes = mh_calloc(h, h->n_buckets, h->node_size);
	h->bitmap = mh_calloc(h, h->n_buckets / 16 + 1, sizeof(uint32_t));
	h->upper_bound = h->n_buckets * 0.7;
	return h;
}

MH_DECL void
_mh(resize)(struct mhash_t *h)
{
	struct mhash_t *s = h->shadow;
#if MH_INCREMENTAL_RESIZE
	uint32_t batch = h->batch;
#endif
	for (uint32_t o = h->resizing; o < h->n_buckets; o++) {
#if MH_INCREMENTAL_RESIZE
		if (batch-- == 0) {
			h->resizing = o;
			return;
		}
#endif
		if (!mh_exist(h, o))
			continue;
		uint32_t n = _mh(put_slot)(s, mh_node_key(mh_slot(h, o)));
		memcpy(mh_slot(s, n), mh_slot(h, o), mh_node_size);
		mh_setexist(s, n);
		s->n_occupied++;
	}
	free(h->nodes);
	free(h->bitmap);
	s->size = h->size;
	memcpy(h, s, sizeof(*h));
	h->resize_cnt++;
}

MH_DECL void
_mh(start_resize)(struct mhash_t *h, uint32_t buckets, uint32_t batch)
{
	if (h->resizing)
		return;
	struct mhash_t *s = h->shadow;
	if (buckets < h->n_buckets)
		buckets = h->n_buckets;
	if (h->size > buckets / 2) {
		for (int k = h->prime; k < __ac_HASH_PRIME_SIZE; k++)
			if (__ac_prime_list[k] > h->size) {
				h->prime = k + 1;
				break;
			}
	}
	h->batch = batch > 0 ? batch : h->n_buckets / (256 * 1024);
	if (h->batch < 256) /* minimum batch is 3 */
		h->batch = 256;
	memcpy(s, h, sizeof(*h));
	s->resizing = 0;
	s->n_buckets = __ac_prime_list[h->prime];
	s->upper_bound = s->n_buckets * 0.7;
	s->n_occupied = 0;
	s->nodes = mh_malloc(h, (size_t)s->n_buckets * h->node_size);
	s->bitmap = mh_calloc(h, s->n_buckets / 16 + 1, sizeof(uint32_t));
	_mh(resize)(h);
}

#  ifdef MH_DEBUG
MH_DECL void
_mh(dump)(struct mhash_t *h)
{
	printf("slots:\n");
	int k = 0;
	for(int i = 0; i < h->n_buckets; i++) {
		if (mh_dirty(h, i) || mh_exist(h, i)) {
			printf("   [%i] ", i);
			if (mh_exist(h, i)) {
				printf("   -> %i", (int)h->p[i].key);
				k++;
			}
			if (mh_dirty(h, i))
				printf(" dirty");
			printf("\n");
		}
	}
	printf("end(%i)\n", k);
}
#  endif
#endif

#undef mh_key_t
#undef mh_val_t
#undef mh_name
#undef mh_hash
#undef mh_eq


#if defined(MH_SOURCE) && !defined(MH_COMMON_SOURCE)
#define MH_COMMON_SOURCE

MH_DECL void
mh_clear(struct mhash_t *h)
{
	free(h->nodes);
	free(h->bitmap);
	h->n_buckets = 3;
	h->prime = 0;
	h->upper_bound = h->n_buckets * 0.7;
	h->nodes = mh_malloc(h, (size_t)h->n_buckets * h->node_size);
	h->bitmap = mh_calloc(h, h->n_buckets / 16 + 1, sizeof(uint32_t));
}

MH_DECL void
mh_destroy(struct mhash_t *h)
{
	mh_free(h, h->shadow);
	mh_free(h, h->bitmap);
	mh_free(h, h->nodes);
	mh_free(h, h);
}

MH_DECL size_t
mh_bytes(struct mhash_t *h)
{
	return h->resizing ? mh_bytes(h->shadow) : 0 +
		sizeof(*h) +
		(size_t)h->n_buckets * h->node_size +
		((size_t)h->n_buckets / 16 + 1) *  sizeof(uint32_t);

}

MH_DECL void *
mh_malloc(struct mhash_t *h, size_t size)
{
	return h->realloc(NULL, size);
}

MH_DECL void *
mh_calloc(struct mhash_t *h, size_t nmemb, size_t size)
{
	size *= nmemb;
	void *ptr = h->realloc(NULL, size);
	memset(ptr, 0, size);
	return ptr;
}

MH_DECL void
mh_free(struct mhash_t *h, void *ptr)
{
	h->realloc(ptr, 0);
}

#endif
