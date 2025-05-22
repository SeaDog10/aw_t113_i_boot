/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * File      : memheap.c
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-04-10     Bernard      first implementation
 * 2012-10-16     Bernard      add the mutex lock for heap object.
 * 2012-12-29     Bernard      memheap can be used as system heap.
 *                             change mutex lock to semaphore lock.
 * 2013-04-10     Bernard      add rt_memheap_realloc function.
 * 2013-05-24     Bernard      fix the rt_memheap_realloc issue.
 * 2013-07-11     Grissiom     fix the memory block splitting issue.
 * 2013-07-15     Grissiom     optimize rt_memheap_realloc
 * 2021-06-03     Flybreak     Fix the crash problem after opening Oz optimization on ac6.
 */
#include "memheap.h"

/* dynamic pool magic and mask */
#define RT_MEMHEAP_MAGIC        0x1ea01ea0
#define RT_MEMHEAP_MASK         0xFFFFFFFE
#define RT_MEMHEAP_USED         0x01
#define RT_MEMHEAP_FREED        0x00

#define RT_MEMHEAP_IS_USED(i)   ((i)->magic & RT_MEMHEAP_USED)
#define RT_MEMHEAP_MINIALLOC    12

#define RT_MEMHEAP_SIZE         RT_ALIGN(sizeof(struct rt_memheap_item), RT_ALIGN_SIZE)
#define MEMITEM_SIZE(item)      ((unsigned long)item->next - (unsigned long)item - RT_MEMHEAP_SIZE)
#define MEMITEM(ptr)            (struct rt_memheap_item*)((uint8_t*)ptr - RT_MEMHEAP_SIZE)

struct rt_memheap system_heap = {0};

static void *rt_memcpy(void *dst, const void *src, uint32_t count)
{
    char *tmp = (char *)dst, *s = (char *)src;
    uint32_t len;

    if (tmp <= s || tmp > (s + count))
    {
        while (count--)
            *tmp ++ = *s ++;
    }
    else
    {
        for (len = count; len > 0; len --)
            tmp[len - 1] = s[len - 1];
    }

    return dst;
}

static void _remove_next_ptr(struct rt_memheap_item *next_ptr)
{
    /* Fix the crash problem after opening Oz optimization on ac6  */
    /* Fix IAR compiler warning  */
    next_ptr->next_free->prev_free = next_ptr->prev_free;
    next_ptr->prev_free->next_free = next_ptr->next_free;
    next_ptr->next->prev = next_ptr->prev;
    next_ptr->prev->next = next_ptr->next;
}

/**
 * @brief   This function initializes a piece of memory called memheap.
 *
 * @note    The initialized memory pool will be:
 *          +-----------------------------------+--------------------------+
 *          | whole freed memory block          | Used Memory Block Tailer |
 *          +-----------------------------------+--------------------------+
 *
 *          block_list --> whole freed memory block
 *
 *          The length of Used Memory Block Tailer is 0,
 *          which is prevents block merging across list
 *
 * @param   memheap is a pointer of the memheap object.
 *
 * @param   name is the name of the memheap.
 *
 * @param   start_addr is the start address of the memheap.
 *
 * @param   size is the size of the memheap.
 *
 * @return  RT_EOK
 */
int rt_memheap_init(struct rt_memheap *memheap,
                         void              *start_addr,
                         uint32_t         size)
{
    struct rt_memheap_item *item;

    // RT_ASSERT(memheap != RT_NULL);
    if (memheap == RT_NULL)
    {
        error("memheap == RT_NULL\r\n");
        return -RT_ERROR;
    }

    memheap->start_addr     = start_addr;
    memheap->pool_size      = RT_ALIGN_DOWN(size, RT_ALIGN_SIZE);
    memheap->available_size = memheap->pool_size - (2 * RT_MEMHEAP_SIZE);
    memheap->max_used_size  = memheap->pool_size - memheap->available_size;

    /* initialize the free list header */
    item            = &(memheap->free_header);
    item->magic     = (RT_MEMHEAP_MAGIC | RT_MEMHEAP_FREED);
    item->pool_ptr  = memheap;
    item->next      = RT_NULL;
    item->prev      = RT_NULL;
    item->next_free = item;
    item->prev_free = item;

    /* set the free list to free list header */
    memheap->free_list = item;

    /* initialize the first big memory block */
    item            = (struct rt_memheap_item *)start_addr;
    item->magic     = (RT_MEMHEAP_MAGIC | RT_MEMHEAP_FREED);
    item->pool_ptr  = memheap;
    item->next      = RT_NULL;
    item->prev      = RT_NULL;
    item->next_free = item;
    item->prev_free = item;

    item->next = (struct rt_memheap_item *)
                 ((uint8_t *)item + memheap->available_size + RT_MEMHEAP_SIZE);
    item->prev = item->next;

    /* block list header */
    memheap->block_list = item;

    /* place the big memory block to free list */
    item->next_free = memheap->free_list->next_free;
    item->prev_free = memheap->free_list;
    memheap->free_list->next_free->prev_free = item;
    memheap->free_list->next_free            = item;

    /* move to the end of memory pool to build a small tailer block,
     * which prevents block merging
     */
    item = item->next;
    /* it's a used memory block */
    item->magic     = (RT_MEMHEAP_MAGIC | RT_MEMHEAP_USED);
    item->pool_ptr  = memheap;
    item->next      = (struct rt_memheap_item *)start_addr;
    item->prev      = (struct rt_memheap_item *)start_addr;
    /* not in free list */
    item->next_free = item->prev_free = RT_NULL;

    /* initialize semaphore lock */
    // rt_sem_init(&(memheap->lock), name, 1, RT_IPC_FLAG_PRIO);
    memheap->locked = RT_FALSE;

    return RT_EOK;
}

/**
 * @brief  Allocate a block of memory with a minimum of 'size' bytes on memheap.
 *
 * @param   heap is a pointer for memheap object.
 *
 * @param   size is the minimum size of the requested block in bytes.
 *
 * @return  the pointer to allocated memory or NULL if no free memory was found.
 */
void *rt_memheap_alloc(struct rt_memheap *heap, uint32_t size)
{
    uint32_t free_size;
    struct rt_memheap_item *header_ptr;

    if (heap == RT_NULL)
    {
        error("heap == RT_NULL\r\n");
        return RT_NULL;
    }

    /* align allocated size */
    size = RT_ALIGN(size, RT_ALIGN_SIZE);
    if (size < RT_MEMHEAP_MINIALLOC)
        size = RT_MEMHEAP_MINIALLOC;

    if (size < heap->available_size)
    {
        /* search on free list */
        free_size = 0;

        /* get the first free memory block */
        header_ptr = heap->free_list->next_free;
        while (header_ptr != heap->free_list && free_size < size)
        {
            /* get current freed memory block size */
            free_size = MEMITEM_SIZE(header_ptr);
            if (free_size < size)
            {
                /* move to next free memory block */
                header_ptr = header_ptr->next_free;
            }
        }

        /* determine if the memory is available. */
        if (free_size >= size)
        {
            /* a block that satisfies the request has been found. */

            /* determine if the block needs to be split. */
            if (free_size >= (size + RT_MEMHEAP_SIZE + RT_MEMHEAP_MINIALLOC))
            {
                struct rt_memheap_item *new_ptr;

                /* split the block. */
                new_ptr = (struct rt_memheap_item *)
                          (((uint8_t *)header_ptr) + size + RT_MEMHEAP_SIZE);

                /* mark the new block as a memory block and freed. */
                new_ptr->magic = (RT_MEMHEAP_MAGIC | RT_MEMHEAP_FREED);

                /* put the pool pointer into the new block. */
                new_ptr->pool_ptr = heap;

                /* break down the block list */
                new_ptr->prev          = header_ptr;
                new_ptr->next          = header_ptr->next;
                header_ptr->next->prev = new_ptr;
                header_ptr->next       = new_ptr;

                /* remove header ptr from free list */
                header_ptr->next_free->prev_free = header_ptr->prev_free;
                header_ptr->prev_free->next_free = header_ptr->next_free;
                header_ptr->next_free = RT_NULL;
                header_ptr->prev_free = RT_NULL;

                /* insert new_ptr to free list */
                new_ptr->next_free = heap->free_list->next_free;
                new_ptr->prev_free = heap->free_list;
                heap->free_list->next_free->prev_free = new_ptr;
                heap->free_list->next_free            = new_ptr;

                /* decrement the available byte count.  */
                heap->available_size = heap->available_size -
                                       size -
                                       RT_MEMHEAP_SIZE;
                if (heap->pool_size - heap->available_size > heap->max_used_size)
                    heap->max_used_size = heap->pool_size - heap->available_size;
            }
            else
            {
                /* decrement the entire free size from the available bytes count. */
                heap->available_size = heap->available_size - free_size;
                if (heap->pool_size - heap->available_size > heap->max_used_size)
                    heap->max_used_size = heap->pool_size - heap->available_size;

                /* remove header_ptr from free list */
                header_ptr->next_free->prev_free = header_ptr->prev_free;
                header_ptr->prev_free->next_free = header_ptr->next_free;
                header_ptr->next_free = RT_NULL;
                header_ptr->prev_free = RT_NULL;
            }

            /* Mark the allocated block as not available. */
            header_ptr->magic = (RT_MEMHEAP_MAGIC | RT_MEMHEAP_USED);

            return (void *)((uint8_t *)header_ptr + RT_MEMHEAP_SIZE);
        }
    }

    /* Return the completion status.  */
    return RT_NULL;
}

/**
 * @brief This function will change the size of previously allocated memory block.
 *
 * @param heap is a pointer to the memheap object, which will reallocate
 *             memory from the block
 *
 * @param ptr is a pointer to start address of memory.
 *
 * @param newsize is the required new size.
 *
 * @return the changed memory block address.
 */
void *rt_memheap_realloc(struct rt_memheap *heap, void *ptr, uint32_t newsize)
{
    uint32_t oldsize;
    struct rt_memheap_item *header_ptr;
    struct rt_memheap_item *new_ptr;

    if (heap == RT_NULL)
    {
        error("heap == RT_NULL\r\n");
        return RT_NULL;
    }

    if (newsize == 0)
    {
        rt_memheap_free(ptr);

        return RT_NULL;
    }
    /* align allocated size */
    newsize = RT_ALIGN(newsize, RT_ALIGN_SIZE);
    if (newsize < RT_MEMHEAP_MINIALLOC)
        newsize = RT_MEMHEAP_MINIALLOC;

    if (ptr == RT_NULL)
    {
        return rt_memheap_alloc(heap, newsize);
    }

    /* get memory block header and get the size of memory block */
    header_ptr = (struct rt_memheap_item *)
                 ((uint8_t *)ptr - RT_MEMHEAP_SIZE);
    oldsize = MEMITEM_SIZE(header_ptr);
    /* re-allocate memory */
    if (newsize > oldsize)
    {
        void *new_ptr;
        struct rt_memheap_item *next_ptr;

        next_ptr = header_ptr->next;

        /* header_ptr should not be the tail */
        if (next_ptr < header_ptr)
        {
            error("next_ptr < header_ptr\r\n");
            while(1);
        }

        /* check whether the following free space is enough to expand */
        if (!RT_MEMHEAP_IS_USED(next_ptr))
        {
            int nextsize;

            nextsize = MEMITEM_SIZE(next_ptr);
            if (next_ptr <= 0)
            {
                error("next_ptr <= 0\r\n");
                while(1);
            }

            /* Here is the ASCII art of the situation that we can make use of
             * the next free node without alloc/memcpy, |*| is the control
             * block:
             *
             *      oldsize           free node
             * |*|-----------|*|----------------------|*|
             *         newsize          >= minialloc
             * |*|----------------|*|-----------------|*|
             */
            if (nextsize + oldsize > newsize + RT_MEMHEAP_MINIALLOC)
            {
                /* decrement the entire free size from the available bytes count. */
                heap->available_size = heap->available_size - (newsize - oldsize);
                if (heap->pool_size - heap->available_size > heap->max_used_size)
                    heap->max_used_size = heap->pool_size - heap->available_size;

                _remove_next_ptr(next_ptr);

                /* build a new one on the right place */
                next_ptr = (struct rt_memheap_item *)((char *)ptr + newsize);

                /* mark the new block as a memory block and freed. */
                next_ptr->magic = (RT_MEMHEAP_MAGIC | RT_MEMHEAP_FREED);

                /* put the pool pointer into the new block. */
                next_ptr->pool_ptr = heap;

                next_ptr->prev          = header_ptr;
                next_ptr->next          = header_ptr->next;
                header_ptr->next->prev = (struct rt_memheap_item *)next_ptr;
                header_ptr->next       = (struct rt_memheap_item *)next_ptr;

                /* insert next_ptr to free list */
                next_ptr->next_free = heap->free_list->next_free;
                next_ptr->prev_free = heap->free_list;
                heap->free_list->next_free->prev_free = (struct rt_memheap_item *)next_ptr;
                heap->free_list->next_free            = (struct rt_memheap_item *)next_ptr;

                return ptr;
            }
        }

        /* re-allocate a memory block */
        new_ptr = (void *)rt_memheap_alloc(heap, newsize);
        if (new_ptr != RT_NULL)
        {
            rt_memcpy(new_ptr, ptr, oldsize < newsize ? oldsize : newsize);
            rt_memheap_free(ptr);
        }

        return new_ptr;
    }

    /* don't split when there is less than one node space left */
    if (newsize + RT_MEMHEAP_SIZE + RT_MEMHEAP_MINIALLOC >= oldsize)
        return ptr;

    /* split the block. */
    new_ptr = (struct rt_memheap_item *)
              (((uint8_t *)header_ptr) + newsize + RT_MEMHEAP_SIZE);

    /* mark the new block as a memory block and freed. */
    new_ptr->magic = (RT_MEMHEAP_MAGIC | RT_MEMHEAP_FREED);
    /* put the pool pointer into the new block. */
    new_ptr->pool_ptr = heap;

    /* break down the block list */
    new_ptr->prev          = header_ptr;
    new_ptr->next          = header_ptr->next;
    header_ptr->next->prev = new_ptr;
    header_ptr->next       = new_ptr;

    /* determine if the block can be merged with the next neighbor. */
    if (!RT_MEMHEAP_IS_USED(new_ptr->next))
    {
        struct rt_memheap_item *free_ptr;

        /* merge block with next neighbor. */
        free_ptr = new_ptr->next;
        heap->available_size = heap->available_size - MEMITEM_SIZE(free_ptr);

        free_ptr->next->prev = new_ptr;
        new_ptr->next   = free_ptr->next;

        /* remove free ptr from free list */
        free_ptr->next_free->prev_free = free_ptr->prev_free;
        free_ptr->prev_free->next_free = free_ptr->next_free;
    }

    /* insert the split block to free list */
    new_ptr->next_free = heap->free_list->next_free;
    new_ptr->prev_free = heap->free_list;
    heap->free_list->next_free->prev_free = new_ptr;
    heap->free_list->next_free            = new_ptr;

    /* increment the available byte count.  */
    heap->available_size = heap->available_size + MEMITEM_SIZE(new_ptr);

    /* return the old memory block */
    return ptr;
}

/**
 * @brief This function will release the allocated memory block by
 *        rt_malloc. The released memory block is taken back to system heap.
 *
 * @param ptr the address of memory which will be released.
 */
void rt_memheap_free(void *ptr)
{
    struct rt_memheap *heap;
    struct rt_memheap_item *header_ptr, *new_ptr;
    int insert_header;

    /* NULL check */
    if (ptr == RT_NULL) return;

    /* set initial status as OK */
    insert_header = RT_TRUE;
    new_ptr       = RT_NULL;
    header_ptr    = (struct rt_memheap_item *)
                    ((uint8_t *)ptr - RT_MEMHEAP_SIZE);

    /* check magic */
    if (header_ptr->magic != (RT_MEMHEAP_MAGIC | RT_MEMHEAP_USED) ||
       (header_ptr->next->magic & RT_MEMHEAP_MASK) != RT_MEMHEAP_MAGIC)
    {
        if (header_ptr->magic != (RT_MEMHEAP_MAGIC | RT_MEMHEAP_USED))
        {
            error("header_ptr->magic != (RT_MEMHEAP_MAGIC | RT_MEMHEAP_USED)\r\n");
            while(1);
        }
        /* check whether this block of memory has been over-written. */
        if ((header_ptr->next->magic & RT_MEMHEAP_MASK)!= RT_MEMHEAP_MAGIC)
        {
            error("(header_ptr->next->magic & RT_MEMHEAP_MASK)!= RT_MEMHEAP_MAGIC\r\n");
            while(1);
        }
    }

    /* get pool ptr */
    heap = header_ptr->pool_ptr;

    // RT_ASSERT(heap);
    if (heap == RT_NULL)
    {
        error("heap == RT_NULL\r\n");
        while(1);
    }

    /* Mark the memory as available. */
    header_ptr->magic = (RT_MEMHEAP_MAGIC | RT_MEMHEAP_FREED);
    /* Adjust the available number of bytes. */
    heap->available_size += MEMITEM_SIZE(header_ptr);

    /* Determine if the block can be merged with the previous neighbor. */
    if (!RT_MEMHEAP_IS_USED(header_ptr->prev))
    {
        /* adjust the available number of bytes. */
        heap->available_size += RT_MEMHEAP_SIZE;

        /* yes, merge block with previous neighbor. */
        (header_ptr->prev)->next = header_ptr->next;
        (header_ptr->next)->prev = header_ptr->prev;

        /* move header pointer to previous. */
        header_ptr = header_ptr->prev;
        /* don't insert header to free list */
        insert_header = RT_FALSE;
    }

    /* determine if the block can be merged with the next neighbor. */
    if (!RT_MEMHEAP_IS_USED(header_ptr->next))
    {
        /* adjust the available number of bytes. */
        heap->available_size += RT_MEMHEAP_SIZE;

        /* merge block with next neighbor. */
        new_ptr = header_ptr->next;

        new_ptr->next->prev = header_ptr;
        header_ptr->next    = new_ptr->next;

        /* remove new ptr from free list */
        new_ptr->next_free->prev_free = new_ptr->prev_free;
        new_ptr->prev_free->next_free = new_ptr->next_free;
    }

    if (insert_header)
    {
        struct rt_memheap_item *n = heap->free_list->next_free;;
#if defined(RT_MEMHEAP_BSET_MODE)
        uint32_t blk_size = MEMITEM_SIZE(header_ptr);
        for (;n != heap->free_list; n = n->next_free)
        {
            uint32_t m = MEMITEM_SIZE(n);
            if (blk_size <= m)
            {
                break;
            }
        }
#endif
        /* no left merge, insert to free list */
        header_ptr->next_free = n;
        header_ptr->prev_free = n->prev_free;
        n->prev_free->next_free = header_ptr;
        n->prev_free = header_ptr;
    }
}

/**
* @brief This function will caculate the total memory, the used memory, and
*        the max used memory.
*
* @param heap is a pointer to the memheap object, which will reallocate
*             memory from the block
*
* @param total is a pointer to get the total size of the memory.
*
* @param used is a pointer to get the size of memory used.
*
* @param max_used is a pointer to get the maximum memory used.
*/
void rt_memheap_info(struct rt_memheap *heap,
                     uint32_t *total,
                     uint32_t *used,
                     uint32_t *max_used)
{
    if (total != RT_NULL)
        *total = heap->pool_size;

    if (used  != RT_NULL)
        *used = heap->pool_size - heap->available_size;

    if (max_used != RT_NULL)
        *max_used = heap->max_used_size;
}
