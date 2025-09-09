/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	cache.h
 * @brief	CACHE operation primitives for libmetal.
 */

#ifndef METAL_CACHE_H__
#define METAL_CACHE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "platform_port.h"

#define VIRTIO_USE_DCACHE

/** \defgroup cache CACHE Interfaces
 *  @{
 */

/**
 * @brief flush specified data cache
 *
 * @param[in] addr start memory logical address
 * @param[in] len  length of memory
 *                 If addr is NULL, and len is 0,
 *                 It will flush the whole data cache.
 */
static inline void metal_cache_flush(void *addr, unsigned int len)
{
    if (addr && len)
    {
        HW_CACHE_FLUSH(addr, len);
    }
}

/**
 * @brief invalidate specified data cache
 *
 * @param[in] addr start memory logical address
 * @param[in] len  length of memory
 *                 If addr is NULL, and len is 0,
 *                 It will invalidate the whole data cache.
 */
static inline void metal_cache_invalidate(void *addr, unsigned int len)
{
    if (addr && len)
    {
        HW_CACHE_INVALIDATE(addr, len);
    }
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
