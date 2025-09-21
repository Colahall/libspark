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

#include "spark/iir_filter.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

static void biquad_process_f32(const float coeff[5], float state[2], float *output,
                               const float *input, size_t frames);


/**
 * @brief Cascade of biquad (SOS) filters, single-precision, mono.
 *
 * Processes @p frames samples through @p stages second-order sections (SOS)
 * in series: the output of stage k feeds stage k+1.
 *
 * ### Per-stage layout (what @p spark_tdf2_biquad_f32() expects)
 * - Coefficients (5 floats per stage): { b0, b1, b2, -a1, -a2 }
 *   - a0 is assumed to be 1.0 and omitted.
 *   - a1 and a2 are negated for convenience.
 *
 * ### State per-stage layout
 * - State (2 floats per stage): { s1, s2 }
 *
 * ### Coefficient memory (flat array)
 * @code
 * coeff[0..5*stages-1] =
 *   // stage 0
 *   b0_0, b1_0, b2_0, -a1_0, -a2_0,
 *   // stage 1
 *   b0_1, b1_1, b2_1, -a1_1, -a2_1,
 *   // stage 2
 *   b0_2, b1_2, b2_2, -a1_2, -a2_2,
 *   ...
 *   // stage (stages-1)
 *   b0_S, b1_S, b2_S, -a1_S, -a2_S
 * @endcode
 *
 * ### State memory (flat array; persists across calls)
 * @code
 * state[0..2*stages-1] =
 *   // stage 0
 *   s1_0, s2_0,
 *   // stage 1
 *   s1_1, s2_1,
 *   // stage 2
 *   s1_2, s2_2,
 *   ...
 *   // stage (stages-1)
 *   s1_S, s2_S
 * @endcode
 *
 * ### Buffer flow
 * - Input:  @p input[0..frames-1]   (read)
 * - Output: @p output[0..frames-1]  (write)
 * - Stage 0 reads @p input and writes @p output.
 * - For k>0, stage k reads from @p output of the previous stage
 *   (in-place cascade).
 *
 * @note Thread safety: @p state belongs to this filter instance; do not share
 * it across threads or filter chains without external synchronization.
 *
 * @note Feedback coefficients (a1, a2) are assumed to be negative.
 *
 * @param[in,out] self Pointer to instance
 */
void spark_sosfilt_f32(spark_sosfilt_f32_t *self)
{
  assert(self);
  int status = spark_block_validate(self, SPARK_FMT_F32 | SPARK_LAYOUT_PLANAR |
                                    SPARK_BLOCK_PROCESS);
  assert(status == SPARK_NOERROR);

  if (status != SPARK_NOERROR) {
    return;
  }

  assert(self->coefficients && self->states && (self->n_stages > 0));

  const uint32_t n_chan = self->header.input.channels;
  const uint32_t n_frames = self->header.input.frames;
  const uint32_t n_stages = self->n_stages;
  const bool share_sos = (self->flags & SPARK_SOSFILT_SHARE_SOS);

  const float *coeff = self->coefficients;
  const float *input = self->header.input.base;
  float *output = self->header.output.base;
  float *states = self->states;

  assert(input && output);

  for (uint32_t chan = 0; chan < n_chan; ++chan) {
    const float *in = input + (chan * n_frames);
    float *out = output + (chan * n_frames);

    /* Start over the count if SOS is shared for all channels */
    if (share_sos) {
      coeff = self->coefficients;
    }

    for (uint32_t stage = 0; stage < n_stages; ++stage) {
      biquad_process_f32(coeff, states, out, in, n_frames);
      coeff += 5;
      states += 2;

      /*
       * For all subsequent stages, the input is the result of the previous
       * stage, which is now in the output buffer.
       */
      in = out;
    }
  }
}

/**
 * @brief Single-precision biquad (SOS) IIR, transposed Direct Form II (TDF-II).
 *
 * Processes @p frames mono samples through one biquad section. The function
 * reads @p input, writes @p output, and updates @p state in place.
 *
 * ### Coefficient layout (5 floats)
 * Coefficients are normalized with a0 = 1 and packed as:
 * @code
 * coeff = { b0, b1, b2, -a1, -a2 }
 * @endcode
 *
 * ### State layout (2 floats)
 * TDF-II delay elements:
 * @code
 * state = { w1, w2 }
 * @endcode
 * These must be preserved across calls (initialize to 0.0f for a cold start).
 *
 * ### Difference equations (TDF-II)
 * @code
 * y  = b0*x + w1
 * w1 = b1*x - a1*y + w2
 * w2 = b2*x - a2*y
 * @endcode
 *
 * ### Buffer flow
 * - Input : input[0..frames-1]   (read)
 * - Output: output[0..frames-1]  (write)
 * - In-place is allowed if `output == input` (reads happen before writes per
 * sample).
 *
 * ### Notes / contracts
 * - @p coeff points to exactly 5 floats: {b0,b1,b2,a1,a2} with a0≡1.
 * - @p state points to exactly 2 floats: {w1,w2}, updated and preserved.
 * - All buffers must be contiguous `float` (32-bit) arrays.
 * - Stable behavior requires a realizable, stable section (|poles| < 1 in
 * z-plane).
 * - If you use this inside a cascade, pass the same @p output as the next
 * stage's input.
 * - For maximum compiler optimization, you may declare the pointers `restrict`
 *   in the definition when aliasing is not possible.
 *
 * @param[in] coeff  Pointer to 5 float32 coefficients {b0,b1,b2,a1,a2}.
 * @param[in,out] state  Pointer to 2 float32 state values {w1,w2} (persist across calls).
 * @param[out] output Pointer to @p frames float32 output samples.
 * @param[in] input  Pointer to @p frames float32 input samples.
 * @param[in] frames Number of mono samples to process.
 */
static void biquad_process_f32(const float coeff[5], float state[2], float *output,
                               const float *input, size_t frames)
{
  // Load coefficients
  const float b0 = coeff[0];
  const float b1 = coeff[1];
  const float b2 = coeff[2];
  const float a1 = coeff[3];
  const float a2 = coeff[4];

  // Load states
  float s1 = state[0];
  float s2 = state[1];

  for (size_t i = 0; i < frames; ++i) {
    float x = input[i];
    float y = (b0 * x) + s1;
    s1 = (b1 * x) + (a1 * y) + s2;
    s2 = (b2 * x) + (a2 * y);
    output[i] = y;
  }

  // Update the state values for the next iteration.
  state[0] = s1;
  state[1] = s2;
}
