/*
 * Copyright (c) 2018, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	assert.h
 * @brief	Assertion support.
 */

#ifndef METAL_ASSERT_H__
#define METAL_ASSERT_H__

#include <assert.h>

/**
 * @brief Assertion macro.
 * @param cond Condition to test.
 */
#define metal_assert(cond) assert(cond)

#endif
