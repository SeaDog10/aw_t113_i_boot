/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	mutex.h
 * @brief	Mutex primitives for libmetal.
 */

#ifndef METAL_MUTEX_H__
#define METAL_MUTEX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "platform_port.h"

/** \defgroup mutex Mutex Interfaces
 *  @{
 */

/**
 * @brief	Initialize a libmetal mutex.
 * @param[in]	mutex	Mutex to initialize.
 */
static inline void metal_mutex_init(metal_mutex_t *mutex)
{
    MUTEEX_INIT(mutex);
}

/**
 * @brief	Deinitialize a libmetal mutex.
 * @param[in]	mutex	Mutex to deinitialize.
 */
static inline void metal_mutex_deinit(metal_mutex_t *mutex)
{
    MUTEEX_DEINIT(mutex);
}

/**
 * @brief	Try to acquire a mutex
 * @param[in]	mutex	Mutex to mutex.
 * @return	0 on failure to acquire, non-zero on success.
 */
static inline int metal_mutex_try_acquire(metal_mutex_t *mutex)
{
    rt_err_t ret;

    ret = MUTEEX_TRYTAKE(mutex);
    if (ret == 0) return 1;

    return 0;
}

/**
 * @brief	Acquire a mutex
 * @param[in]	mutex	Mutex to mutex.
 */
static inline void metal_mutex_acquire(metal_mutex_t *mutex)
{
    MUTEEX_TAKE(mutex, -1);
}

/**
 * @brief	Release a previously acquired mutex.
 * @param[in]	mutex	Mutex to mutex.
 * @see metal_mutex_try_acquire, metal_mutex_acquire
 */
static inline void metal_mutex_release(metal_mutex_t *mutex)
{
    MUTEEX_RELEASE(mutex);
}

/**
 * @brief	Checked if a mutex has been acquired.
 * @param[in]	mutex	mutex to check.
 * @see metal_mutex_try_acquire, metal_mutex_acquire
 */
static inline int metal_mutex_is_acquired(metal_mutex_t *mutex)
{
    return (MUTEEX_GET_OWNER(mutex) != NULL);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif
