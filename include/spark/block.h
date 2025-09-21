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

#ifndef LIBSPARK_BLOCK_H_
#define LIBSPARK_BLOCK_H_

#include <spark/libspark_api.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buffer format and layout bitfield.
 *
 * Bits [0..3] encode the sample format; bits [4..7] encode the memory layout.
 * Exactly one `SPARK_FMT_*` and one `SPARK_LAYOUT_*` must be set at a time.
 *
 * Layout semantics (single @ref spark_buffer_t::base pointer):
 * - @ref SPARK_LAYOUT_INTERLEAVED:
 *     `base` points to `frames * channels` samples in frame-major order:
 *     frame n = [c0, c1, …, c(C-1)], then frame n+1, etc.
 * - @ref SPARK_LAYOUT_PLANAR:
 *     `base` points to channel-major **contiguous planes** (not ptr-to-ptr):
 *     channel k starts at `((T*)base) + k * frames`. Each plane has `frames` samples.
 *
 * @note Use `spark_buffer_get_format(flags)` / `spark_buffer_get_layout(flags)`
 *       to extract fields, and `spark_buffer_bytes_per_sample()` for element size.
 */
enum spark_buffer_flags {
  /* Formats (bits [0..3]) */
  SPARK_FMT_INVALID = 0x00u, /**< Invalid or unspecified format. */
  SPARK_FMT_I16 = 0x01u,     /**< 16-bit signed integer. */
  SPARK_FMT_I32 = 0x02u,     /**< 32-bit signed integer. */
  SPARK_FMT_F32 = 0x03u,     /**< 32-bit floating-point. */
  SPARK_FMT_F64 = 0x04u,     /**< 64-bit floating-point. */
  SPARK_FMT_MASK = 0x0Fu,    /**< Mask for format bits. */

  /* Layouts (bits [4..7]) */
  SPARK_LAYOUT_INVALID = 0x0u << 4,     /**< Invalid or unspecified layout. */
  SPARK_LAYOUT_INTERLEAVED = 0x1u << 4, /**< Single buffer;
                                            frame-major: [L,R,L,R,...]. */
  SPARK_LAYOUT_PLANAR = 0x2u << 4,      /**< Single buffer;
                                             channel planes: [L(0..N-1)][R(0..N-1)]… */
  SPARK_LAYOUT_MASK = 0x0Fu << 4        /**< Mask for layout bits. */
};

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static_assert((SPARK_FMT_MASK & SPARK_LAYOUT_MASK) == 0, "fmt/layout bits overlap");
#endif

/**
 * @brief Block type field (occupies bits [8..10]) used to declare I/O semantics.
 *
 * Exactly one of these values must be present in the flags word at any time.
 * Combine with one `SPARK_FMT_*` and one `SPARK_LAYOUT_*` when composing
 * operation flags. Use @ref SPARK_BLOCK_TYPE_MASK to extract the type.
 *
 * @code
 * // Example: F32 + INTERLEAVED + PROCESS
 * uint32_t flags = SPARK_FMT_F32 | SPARK_LAYOUT_INTERLEAVED | SPARK_BLOCK_PROCESS;
 * @endcode
 */
enum spark_block_type {
  /**
   * Invalid block type
   */
  SPARK_BLOCK_INVALID = 0,
  /** Process block:
   *  - Input and output must be *similar* (same format, layout, channels, frames).
   *  - Both input.base and output.base must be non-NULL.
   */
  SPARK_BLOCK_PROCESS = 0x1U << 8,

  /** Convert block:
   *  - Input must match the required format/layout.
   *  - Input.base and output.base must be non-NULL.
   *  - Frame counts may differ (channel policy is kernel/operation specific).
   */
  SPARK_BLOCK_CONVERT = 0x2U << 8,

  /** Source block:
   *  - Produces data only (no input required).
   *  - Output must match the required format/layout and have non-zero shape with non-NULL
   *    base.
   */
  SPARK_BLOCK_SOURCE = 0x3U << 8,

  /** Sink block:
   *  - Consumes data only (no output required).
   *  - Input must match the required format/layout and have non-zero shape with non-NULL
   *    base.
   */
  SPARK_BLOCK_SINK = 0x4U << 8,

  /** Mask covering the block type bit-field (bits [8..10]). */
  SPARK_BLOCK_TYPE_MASK = 0x7U << 8
};

/**
 * @brief Error codes returned by libspark validation and processing routines.
 *
 * Conventions:
 * - Success is `SPARK_NOERROR` (0). Any non-zero value indicates failure.
 * - Prefer the most specific code available (INPUT/OUTPUT vs. BLOCK).
 *
 * Typical usage:
 * - Return @ref SPARK_ERR_INVALID_PARAM for bad pointers, missing required flags,
 *   or otherwise malformed arguments detected at the API boundary.
 * - Return @ref SPARK_ERR_INVALID_SIZE when `struct_size` is too small.
 * - Return @ref SPARK_ERR_INVALID_ABI when `abi_version` mismatches the library.
 * - Return @ref SPARK_ERR_INVALID_INPUT / @ref SPARK_ERR_INVALID_OUTPUT for
 *   buffer-specific issues (wrong format/layout, zero channels/frames, NULL base
 *   where required).
 * - Return @ref SPARK_ERR_INVALID_BLOCK for cross-buffer or block-type problems
 *   (e.g., PROCESS requires input/output similarity and both bases present).
 */
enum spark_block_error {
  SPARK_NOERROR = 0, /**< Success. */

  SPARK_ERR_INVALID_PARAM = 1, /**< Malformed API usage:
                                NULL args, missing/invalid flags, etc. */
  SPARK_ERR_INVALID_SIZE = 2,  /**< `struct_size` too small for the expected type. */
  SPARK_ERR_INVALID_ABI = 3,   /**< `abi_version` does not match `SPARK_ABI_VERSION`. */

  SPARK_ERR_INVALID_INPUT = 4,  /**< Input buffer invalid: wrong fmt/layout, zero dims, or
                                   NULL base (when required). */
  SPARK_ERR_INVALID_OUTPUT = 5, /**< Output buffer invalid: wrong fmt/layout, zero dims,
                                   or NULL base (when required). */

  SPARK_ERR_INVALID_BLOCK = 6, /**< Block-level constraint violated: e.g., PROCESS
                                  similarity, required bases/layouts. */
};

/**
 * @brief Descriptor of a multichannel audio buffer (format + layout in @ref flags).
 *
 * The buffer’s sample format and memory layout are encoded in @ref flags using
 * exactly one `SPARK_FMT_*` and one `SPARK_LAYOUT_*` value.
 *
 * @details
 *
 * ## Layouts (single @ref base pointer):
 * - @ref SPARK_LAYOUT_INTERLEAVED
 *   `base` points to `frames * channels` samples in *frame-major* order:
 *   frame n = `[c0, c1, …, c(C-1)]`, then frame n+1, etc.
 *
 * - @ref SPARK_LAYOUT_PLANAR
 *   `base` points to *channel-major contiguous planes* (not pointer-to-pointer):
 *   channel k starts at `((T*)base) + k * frames`, each plane holds `frames` samples.
 *
 * This type does **not** model pointer-to-pointer planar or custom strides. If you
 * need those, add a separate descriptor or pack into a contiguous view.
 *
 * @note
 * - @ref frames is the number of samples **per channel**.
 * - Ownership and constness are not encoded here; kernels should treat the input
 *   buffer as read-only by convention.
 *
 * - Use @ref spark_buffer_get_format and @ref spark_buffer_get_layout to decode
 *   @ref flags, and @ref spark_buffer_bytes_per_sample to obtain the element size.
 *
 * @see SPARK_FMT_MASK, SPARK_LAYOUT_MASK
 */
typedef struct spark_buffer {
  void *base;        /**< Base pointer to the sample data (see layout semantics). */
  uint32_t channels; /**< The number of audio channels */
  uint32_t frames;   /**< Number of frames per channel */
  uint32_t flags;    /**< Combined flags: one SPARK_FMT_* and one SPARK_LAYOUT_*. */
} spark_buffer_t;

/**
 * @brief Descriptor for processing multichannel audio.
 *
 * A @ref spark_block_t bundles the input and output buffer descriptors plus
 * ABI/versioning for forward compatibility. The sample format and memory
 * layout are encoded in each buffer’s `flags` field (see @ref spark_buffer_t,
 * @ref spark_buffer_get_format, and @ref spark_buffer_get_layout).
 *
 * @details
 * - The caller must set `abi_version` to @ref SPARK_ABI_VERSION and
 *   `struct_size` to `sizeof(spark_block_t)` before use.
 * - For interleaved layouts, `buffer.base` points to frames × channels samples
 *   in frame-major order. For planar layouts, `buffer.base` points to
 *   channel-major contiguous planes (channel k starts at
 *   `base + k * frames`). Pointer-to-pointer planar is not represented.
 * - Whether input/output bases may alias is kernel-dependent; validate with
 *   @ref spark_block_validate for a specific operation (PROCESS/CONVERT/SOURCE/SINK).
 *
 * @see spark_buffer_t
 * @see spark_block_validate
 */
typedef struct spark_block {
  /* ABI/versioning */
  uint32_t abi_version; /**< Must equal @ref SPARK_ABI_VERSION
                             (from <spark/version.h>). */
  uint32_t struct_size; /**< Must be initialized to `sizeof(spark_block_t)` or more by the
                             caller. */

  /* Input and output buffers */
  spark_buffer_t input; /**< Input buffer descriptor (treated as read-only by kernels). */
  spark_buffer_t output; /**< Output buffer descriptor (written by kernels). */

} spark_block_t;


/**
 * @brief Extracts the layout from a flags field.
 * @param[in] flags The combined flags value.
 * @return The layout value (e.g., SPARK_LAYOUT_INTERLEAVED).
 */
static inline uint32_t spark_buffer_get_layout(uint32_t flags)
{
  return flags & SPARK_LAYOUT_MASK;
}

/**
 * @brief Extracts the format from a flags field.
 * @param[in] flags The combined flags value.
 * @return The format value (e.g., SPARK_FMT_F32).
 */
static inline uint32_t spark_buffer_get_format(uint32_t flags)
{
  return flags & SPARK_FMT_MASK;
}

/**
 * @brief Extract the block type from flags
 *
 * @param[in] flags The combined flags value.
 * @return uint32_t The block type value (e.g. SPARK_BLOCK_PROCESS)
 */
static inline uint32_t spark_block_get_type(uint32_t flags)
{
  return (flags & SPARK_BLOCK_TYPE_MASK);
}

/**
 * @brief Return the size, in bytes, of one sample for the buffer's format.
 *
 * Inspects @p buf->flags and maps the `SPARK_FMT_*` value to its byte width.
 *
 * @param[in] buf  Non-NULL pointer to a buffer whose format is encoded
 *                 in @p buf->flags (one of `SPARK_FMT_I16`, `SPARK_FMT_I32`,
 *                 `SPARK_FMT_F32`, `SPARK_FMT_F64`).
 * @return The byte size of a single sample (2, 4, or 8), or 0 if the format
 *         bits are invalid/unsupported.
 *
 * @see spark_buffer_get_format
 */
static inline size_t spark_buffer_bytes_per_sample(const spark_buffer_t *buf)
{
  if (!buf)
    return 0;
  switch (spark_buffer_get_format(buf->flags)) {
  case SPARK_FMT_I16:
    return sizeof(int16_t);
  case SPARK_FMT_I32:
    return sizeof(int32_t);
  case SPARK_FMT_F32:
    return sizeof(float);
  case SPARK_FMT_F64:
    return sizeof(double);
  default:
    return 0;
  }
}

/**
 * @brief Compare two buffers for *metadata* equivalence.
 *
 * Returns `true` iff @p a and @p b describe the **same format, layout,
 * channel count, and frame count**. The underlying data pointers are
 * **not** compared—this is intentionally a shape/flags comparison.
 *
 * Special cases:
 * - If `a == b`, the function returns `true` (same object).
 * - If either pointer is `NULL` (and they are not the same), returns `false`.
 *
 * This function does **not** validate that the format/layout bits are valid,
 * nor that channels/frames are non-zero. Use this for quick compatibility checks;
 * perform full validation elsewhere (e.g., in `spark_block_validate()`).
 *
 * @param[in] a  First buffer descriptor (may be NULL).
 * @param[in] b  Second buffer descriptor (may be NULL).
 * @retval true  If format, layout, channels, and frames all match.
 * @retval false Otherwise, or if either pointer is NULL (and not identical).
 */
static inline bool spark_buffer_is_similar(const spark_buffer_t *a,
                                           const spark_buffer_t *b)
{
  if (a == b)
    return true;
  if (!a || !b)
    return false;

  return (spark_buffer_get_format(a->flags) == spark_buffer_get_format(b->flags)) &&
         (spark_buffer_get_layout(a->flags) == spark_buffer_get_layout(b->flags)) &&
         (a->channels == b->channels) && (a->frames == b->frames);
}

/**
 * @brief Check if a buffer descriptor points to a valid, non-empty buffer.
 *
 * This is a convenience function to verify that the buffer pointer itself is not NULL,
 * its base data pointer is not NULL, and its dimensions are non-zero.
 *
 * @param[in] buf  The buffer descriptor to check.
 * @retval true   If the buffer appears to be valid for processing.
 * @retval false  Otherwise.
 */
static inline bool spark_buffer_is_valid(const spark_buffer_t *buf)
{
  return (buf) && (buf->base) && (buf->channels > 0) && (buf->frames > 0);
}

/**
 * @brief Check if a buffer's metadata matches a required format and layout.
 *
 * @param[in] buf     The buffer descriptor to check.
 * @param[in] fmt     The required format flag (e.g., `SPARK_FMT_F32`).
 * @param[in] layout  The required layout flag (e.g., `SPARK_LAYOUT_PLANAR`).
 * @retval true      If the buffer's format and layout match the required flags.
 * @retval false     Otherwise, or if `buf` is NULL.
 */
static inline bool spark_buffer_check_type(const spark_buffer_t *buf, uint32_t fmt,
                                           uint32_t layout)
{
  if (!buf)
    return false;

  return (spark_buffer_get_format(buf->flags) == fmt) &&
         (spark_buffer_get_layout(buf->flags) == layout);
}

/** API Functions **/

LIBSPARK_API int spark_block_validate(const void *p_block, uint32_t flags);
LIBSPARK_API const char *spark_strerror(int err);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LIBSPARK_BLOCK_H_ */
