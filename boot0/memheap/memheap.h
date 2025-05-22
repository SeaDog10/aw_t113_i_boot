#ifndef __MEMHEAP_H__
#define __MEMHEAP_H__

#include "board.h"
#include "main.h"
#include "debug.h"

#define RT_ALIGN_SIZE                   4
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

#define RT_NULL                         (0)

/* boolean type definitions */
#define RT_TRUE                         1               /**< boolean true  */
#define RT_FALSE                        0               /**< boolean fails */

#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        1               /**< A generic error happens */
#define RT_ETIMEOUT                     2               /**< Timed out */
#define RT_EFULL                        3               /**< The resource is full */
#define RT_EEMPTY                       4               /**< The resource is empty */
#define RT_ENOMEM                       5               /**< No memory */
#define RT_ENOSYS                       6               /**< No system */
#define RT_EBUSY                        7               /**< Busy */
#define RT_EIO                          8               /**< IO error */
#define RT_EINTR                        9               /**< Interrupted system call */
#define RT_EINVAL                       10              /**< Invalid argument */

extern struct rt_memheap system_heap;

/**
 * memory item on the heap
 */
struct rt_memheap_item
{
    uint32_t             magic;                      /**< magic number for memheap */
    struct rt_memheap      *pool_ptr;                   /**< point of pool */

    struct rt_memheap_item *next;                       /**< next memheap item */
    struct rt_memheap_item *prev;                       /**< prev memheap item */

    struct rt_memheap_item *next_free;                  /**< next free memheap item */
    struct rt_memheap_item *prev_free;                  /**< prev free memheap item */
};

/**
 * Base structure of memory heap object
 */
struct rt_memheap
{
    void                   *start_addr;                 /**< pool start address and size */
    uint32_t               pool_size;                  /**< pool size */
    uint32_t               available_size;             /**< available size */
    uint32_t               max_used_size;              /**< maximum allocated size */
    struct rt_memheap_item *block_list;                 /**< used block list */
    struct rt_memheap_item *free_list;                  /**< free block list */
    struct rt_memheap_item  free_header;                /**< free block list header */
    int                      locked;                     /**< External lock mark */
};

int rt_memheap_init(struct rt_memheap *memheap, void *start_addr, uint32_t size);
int rt_memheap_detach(struct rt_memheap *heap);
void *rt_memheap_alloc(struct rt_memheap *heap, uint32_t size);
void *rt_memheap_realloc(struct rt_memheap *heap, void *ptr, uint32_t newsize);
void rt_memheap_free(void *ptr);
void rt_memheap_info(struct rt_memheap *heap, uint32_t *total, uint32_t *used, uint32_t *max_used);

#endif
