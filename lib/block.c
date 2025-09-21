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

#include "spark/block.h"


/**
 * @brief Validate a @ref spark_block_t against required flags.
 *
 * This checks ABI/size, presence of required format & layout in @p flags,
 * and then applies per–block-type rules:
 *
 * - **SPARK_BLOCK_PROCESS**:
 *   - `input` and `output` must be *similar* (same channels, frames, fmt, layout).
 *   - `input.base` and `output.base` must be non-NULL.
 *   - Both buffers' fmt/layout must match the required fmt/layout in @p flags.
 *
 * - **SPARK_BLOCK_CONVERT**:
 *   - `input` must have required fmt/layout and non-zero channels/frames.
 *   - `input.base` **and** `output.base` must be non-NULL (conversion produces data).
 *   - Frame counts may differ (channels policy up to the caller—document your choice).
 *
 * - **SPARK_BLOCK_SOURCE**:
 *   - `output` must have required fmt/layout, non-zero channels/frames, and non-NULL
 *      base.
 *   - `input` is ignored (may be NULL/zero).
 *
 * - **SPARK_BLOCK_SINK**:
 *   - `input` must have required fmt/layout, non-zero channels/frames, and non-NULL base.
 *   - `output` is ignored (may be NULL/zero).
 *
 * @param p_block Pointer to the block to validate (may be NULL).
 * @param flags   Required flags for this operation:
 *                combine exactly one `SPARK_FMT_*`, one `SPARK_LAYOUT_*`,
 *                and exactly one block type (`SPARK_BLOCK_*` in the type field).
 *
 * @retval SPARK_NOERROR              on success
 * @retval SPARK_ERR_INVALID_PARAM    if @p p_block is NULL, or required fmt/layout
 *                                      missing
 * @retval SPARK_ERR_INVALID_SIZE     if `struct_size` is too small
 * @retval SPARK_ERR_INVALID_ABI      if `abi_version` mismatches `SPARK_ABI_VERSION`
 * @retval SPARK_ERR_INVALID_BLOCK    on shape/pointer issues for the selected block type
 * @retval SPARK_ERR_INVALID_INPUT    if input fmt/layout/pointer is invalid for the type
 * @retval SPARK_ERR_INVALID_OUTPUT   if output fmt/layout/pointer is invalid for the type
 */
int spark_block_validate(const void *p_block, uint32_t flags)
{
  const spark_block_t *block = (const spark_block_t *)p_block;
  if (!block)
    return SPARK_ERR_INVALID_PARAM;

  if (block->struct_size < sizeof(spark_block_t))
    return SPARK_ERR_INVALID_SIZE;

  if (block->abi_version != SPARK_ABI_VERSION)
    return SPARK_ERR_INVALID_ABI;

  // Ensure required fmt/layout are present in 'flags'
  const uint32_t req_fmt = spark_buffer_get_format(flags);
  const uint32_t req_layout = spark_buffer_get_layout(flags);
  const uint32_t block_type = spark_block_get_type(flags);

  /* Guard against missing required fmt/layout in flags */
  if (req_fmt == SPARK_FMT_INVALID || req_layout == SPARK_LAYOUT_INVALID ||
      block_type == SPARK_BLOCK_INVALID)
    return SPARK_ERR_INVALID_PARAM;

  if (block_type == SPARK_BLOCK_PROCESS) {
    /* Shapes must match */
    if (!spark_buffer_is_similar(&block->input, &block->output))
      return SPARK_ERR_INVALID_BLOCK;

    /* Input and output buffers must be non-null. */
    if (!spark_buffer_is_valid(&block->input))
      return SPARK_ERR_INVALID_INPUT;

    if (!spark_buffer_is_valid(&block->output))
      return SPARK_ERR_INVALID_OUTPUT;

    /* Input must match required fmt/layout */
    if (!spark_buffer_check_type(&block->input, req_fmt, req_layout))
      return SPARK_ERR_INVALID_INPUT;

    /* Output must match required fmt/layout */
    if (!spark_buffer_check_type(&block->output, req_fmt, req_layout))
      return SPARK_ERR_INVALID_OUTPUT;

  } else if (block_type == SPARK_BLOCK_CONVERT) {
    /* Check if valid input buffer */
    if (!spark_buffer_is_valid(&block->input))
      return SPARK_ERR_INVALID_INPUT;

    /* Check if valid output buffer */
    if (!spark_buffer_is_valid(&block->output))
      return SPARK_ERR_INVALID_OUTPUT;

    /* Input must match required fmt/layout */
    if (!spark_buffer_check_type(&block->input, req_fmt, req_layout))
      return SPARK_ERR_INVALID_INPUT;

  } else if (block_type == SPARK_BLOCK_SOURCE) {
    /* Source produces output only */

    /* Check if valid output buffer */
    if (!spark_buffer_is_valid(&block->output))
      return SPARK_ERR_INVALID_OUTPUT;

    /* Output must match required fmt/layout */
    if (!spark_buffer_check_type(&block->output, req_fmt, req_layout))
      return SPARK_ERR_INVALID_OUTPUT;

  } else if (block_type == SPARK_BLOCK_SINK) {
    /* Sink consumes input only */

    /* Check if valid input buffer */
    if (!spark_buffer_is_valid(&block->input))
      return SPARK_ERR_INVALID_INPUT;

    /* Input must match required fmt/layout */
    if (!spark_buffer_check_type(&block->input, req_fmt, req_layout))
      return SPARK_ERR_INVALID_INPUT;

  } else {
    /* Unknown/invalid block type */
    return SPARK_ERR_INVALID_BLOCK;
  }

  /* All OK */
  return SPARK_NOERROR;
}

/**
 * @brief Get a human-readable string for a libspark error code.
 *
 * This function provides a descriptive string for any error code returned
 * by the library, which is useful for logging and debugging.
 *
 * @param[in] err The error code from a `spark_block_error` enum value.
 * @return A constant string describing the error. Returns "unknown error"
 * for any integer value not defined in the enum.
 */
const char *spark_strerror(int err)
{
  switch (err) {
  case SPARK_NOERROR:
    return "no error";
  case SPARK_ERR_INVALID_PARAM:
    return "invalid parameter";
  case SPARK_ERR_INVALID_SIZE:
    return "invalid size";
  case SPARK_ERR_INVALID_ABI:
    return "invalid ABI version";
  case SPARK_ERR_INVALID_INPUT:
    return "invalid input buffer";
  case SPARK_ERR_INVALID_OUTPUT:
    return "invalid output buffer";
  case SPARK_ERR_INVALID_BLOCK:
    return "invalid block constraints";
  default:
    return "unknown error";
  }
}