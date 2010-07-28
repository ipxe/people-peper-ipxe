#ifndef _IPXE_MALLOC_H
#define _IPXE_MALLOC_H

#include <stdint.h>

/** @file
 *
 * Dynamic memory allocation
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

/*
 * Prototypes for the standard functions (malloc() et al) are in
 * stdlib.h.  Include <ipxe/malloc.h> only if you need the
 * non-standard functions, such as malloc_dma().
 *
 */
#include <stdlib.h>
#include <ipxe/tables.h>
#include <ipxe/list.h>

#include <valgrind/memcheck.h>

/** A memory pool */
struct memory_pool {
	/** List of free memory blocks */
	struct list_head free_blocks;
};

extern size_t freemem;

/**
 * Set a new memory pool for DMA allocations.
 *
 * @v   new_pool New DMA pool
 * @ret old_pool Old DMA pool
 */
extern struct memory_pool *set_dma_pool ( struct memory_pool *new_pool );

extern void mpopulate ( struct memory_pool *pool, void *start, size_t len );
extern void mdumpfree ( struct memory_pool *pool );

/**
 * Allocate memory for DMA
 *
 * @v size		Requested size
 * @v align		Physical alignment
 * @ret ptr		Memory, or NULL
 *
 * Allocates physically-aligned memory for DMA.
 *
 * @c align must be a power of two.  @c size may not be zero.
 */
extern void * __malloc malloc_dma ( size_t size, size_t phys_align );

/**
 * Free memory allocated with malloc_dma()
 *
 * @v ptr		Memory allocated by malloc_dma(), or NULL
 * @v size		Size of memory, as passed to malloc_dma()
 *
 * Memory allocated with malloc_dma() can only be freed with
 * free_dma(); it cannot be freed with the standard free().
 *
 * If @c ptr is NULL, no action is taken.
 */
extern void free_dma ( void *ptr, size_t size );

/** A cache discarder */
struct cache_discarder {
	/**
	 * Discard some cached data
	 *
	 * @ret discarded	Number of cached items discarded
	 */
	unsigned int ( * discard ) ( void );
};

/** Cache discarder table */
#define CACHE_DISCARDERS __table ( struct cache_discarder, "cache_discarders" )

/** Declare a cache discarder */
#define __cache_discarder __table_entry ( CACHE_DISCARDERS, 01 )

#endif /* _IPXE_MALLOC_H */
