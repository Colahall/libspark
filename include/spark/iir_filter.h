/*
 * Copyright (c) 2025 Colahall, LLC <about@colahall.io>.
 *
 * This File is part of libspark (see https://colahall.io/libspark).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * “Software”), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifndef LIBSPARK_IIR_FILTER_H_
#define LIBSPARK_IIR_FILTER_H_

#include "spark/block.h"
#include "spark/libspark_api.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines how filter coefficients are applied across channels.
 *
 * This enumeration provides flags to control the coefficient memory layout
 * and behavior in a multichannel context.
 */
enum spark_sosfilt_flags {
  /**
   * Each channel uses a unique set of SOS coefficients. The coefficients
   * array must be structured as `[channel0_sos, channel1_sos, ...]`,
   * where each `channel_sos` block contains `n_stages * 6` values.
   */
  SPARK_SOSFILT_INDEPENDENT_SOS = 0,

  /**
   * All channels share the same set of SOS coefficients. The coefficients
   * array should contain only one set of `n_stages * 6` values.
   */
  SPARK_SOSFILT_SHARE_SOS = 1,
};

/**
 * @brief Parameters for a cascaded second-order section (SOS) filter.
 *
 * This struct encapsulates all necessary data for an IIR filtering operation,
 * including the filter's persistent state (coefficients, memory) and the
 * I/O buffers for a single processing block.
 */
typedef struct spark_sosfilt_f32 {
  /**
   * @param[in,out] header Block header structure
   */
  spark_block_t header;

  /**
   * @param[in] coefficients Pointer to the biquad filter coefficients.
   * The layout is determined by the `flags` field.
   */
  const float *coefficients;

  /**
   * @param[in,out] states Pointer to the filter's state memory. This array
   * is read from and written to during processing. Its size must be at least
   * `io.n_channels * n_stages * 2`.
   */
  float *states;

  /**
   * @param[in] n_stages The number of second-order sections in the cascade.
   */
  uint32_t n_stages;

  /**
   * @param[in] flags A flag from the ::spark_sosfilt_flags enum that
   * defines how coefficients are shared across channels.
   */
  uint32_t flags;

} spark_sosfilt_f32_t;


/** Public API functions **/
LIBSPARK_API void spark_sosfilt_f32(spark_sosfilt_f32_t *self);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIBSPARK_IIR_FILTER_H_ */