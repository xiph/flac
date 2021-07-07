#include "private/cpu.h"

#ifndef FLAC__INTEGER_ONLY_LIBRARY
#ifndef FLAC__NO_ASM
#if defined FLAC__CPU_AARCH64 && FLAC__HAS_A64NEONINTRIN
#include "private/lpc.h"
#include "FLAC/assert.h"
#include "FLAC/format.h"
#include <arm_neon.h>

/*
*   Index   [0 , 1 , 2 , 3]
*   Mask    [3 , 0 , 1 , 2]
*   Vec     [1 , 2 , 3 , 4]
*   Result  [4 , 1 , 2 , 3]
*/
inline float32x4_t shufffleVector_3012(float32x4_t vec)
{
    float32_t lane0 = vgetq_lane_f32(vec, 0);
    float32_t lane1 = vgetq_lane_f32(vec, 1);
    float32_t lane2 = vgetq_lane_f32(vec, 2);
    float32_t lane3 = vgetq_lane_f32(vec, 3);

    vec = vsetq_lane_f32(lane3, vec, 0);
    vec = vsetq_lane_f32(lane0, vec, 1);
    vec = vsetq_lane_f32(lane1, vec, 2);
    vec = vsetq_lane_f32(lane2, vec, 3);

    return vec;
}
/*
*   Source  [4 , 4 , 4 , 4]
*   Dest    [1 , 2 , 3 , 4]
*   Result  [4 , 2 , 3 , 4]
*/
inline float32x4_t copyLane(float32x4_t dest, float32x4_t source)
{
    float32_t lane0 = vgetq_lane_f32(source, 0);
    dest = vsetq_lane_f32(lane0, dest, 0);
    return dest;
}

void FLAC__lpc_compute_autocorrelation_intrin_neon_lag_4(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
    // This function calculates autocorrelation with NEON
    // vector functions up to a lag of 4 (or max LPC order of 3)
    // For comments, please see FLAC__lpc_compute_autocorrelation_intrin_neon_lag_16
    int i;
    int limit = data_len - 4;
    float64x2_t sum0 = vdupq_n_f64(0.0f);
    float64x2_t sum1 = vdupq_n_f64(0.0f);
    float32x4_t sumtemp = vdupq_n_f32(0.0f);

    (void)lag;
    FLAC__ASSERT(lag <= 4);

    for (i = 0; i <= limit; i++)
    {
        float32x4_t d, d0;
        d0 = vld1q_f32(data + i);
        d = vdupq_n_f32(vgetq_lane_f32(d0, 0));

        sumtemp = vmulq_f32(d0, d);
        sum0 = vaddq_f64(sum0, vcvt_f64_f32(vget_low_f32(sumtemp)));
        sum1 = vaddq_f64(sum1, vcvt_f64_f32(vget_high_f32(sumtemp)));
    }

    {
        float32x4_t d0 = vdupq_n_f32(0.0f);
        limit++;
        if (limit < 0)
            limit = 0;

        for (i = data_len - 1; i >= limit; i--)
        {
            float32x4_t d = vdupq_n_f32(data[i]);
            d0 = shufffleVector_3012(d0);
            d0 = copyLane(d0, d);

            sumtemp = vmulq_f32(d0, d);
            sum0 = vaddq_f64(sum0, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum1 = vaddq_f64(sum1, vcvt_f64_f32(vget_high_f32(sumtemp)));
        }
    }
    vst1q_f64(autoc, sum0);
    vst1q_f64(autoc + 2, sum1);
}

void FLAC__lpc_compute_autocorrelation_intrin_neon_lag_8(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
    // This function calculates autocorrelation with NEON
    // vector functions up to a lag of 8 (or max LPC order of 7)
    // For comments, please see FLAC__lpc_compute_autocorrelation_intrin_neon_lag_16
    int i;
    int limit = data_len - 8;
    float64x2_t sum0 = vdupq_n_f64(0.0f);
    float64x2_t sum1 = vdupq_n_f64(0.0f);
    float64x2_t sum2 = vdupq_n_f64(0.0f);
    float64x2_t sum3 = vdupq_n_f64(0.0f);
    float32x4_t sumtemp = vdupq_n_f32(0.0f);

    (void)lag;
    FLAC__ASSERT(lag <= 8);

    for (i = 0; i <= limit; i++)
    {
        float32x4_t d, d0, d1;
        d0 = vld1q_f32(data + i);
        d1 = vld1q_f32(data + i + 4);
        d = vdupq_n_f32(vgetq_lane_f32(d0, 0));

        sumtemp = vmulq_f32(d0, d);
        sum0 = vaddq_f64(sum0, vcvt_f64_f32(vget_low_f32(sumtemp)));
        sum1 = vaddq_f64(sum1, vcvt_f64_f32(vget_high_f32(sumtemp)));
        sumtemp = vmulq_f32(d1, d);
        sum2 = vaddq_f64(sum2, vcvt_f64_f32(vget_low_f32(sumtemp)));
        sum3 = vaddq_f64(sum3, vcvt_f64_f32(vget_high_f32(sumtemp)));
    }

    {
        float32x4_t d0 = vdupq_n_f32(0.0f);
        float32x4_t d1 = vdupq_n_f32(0.0f);
        limit++;
        if (limit < 0)
            limit = 0;

        for (i = data_len - 1; i >= limit; i--)
        {
            float32x4_t d = vdupq_n_f32(data[i]);
            d1 = shufffleVector_3012(d1);
            d0 = shufffleVector_3012(d0);
            d1 = copyLane(d1, d0);
            d0 = copyLane(d0, d);

            sumtemp = vmulq_f32(d0, d);
            sum0 = vaddq_f64(sum0, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum1 = vaddq_f64(sum1, vcvt_f64_f32(vget_high_f32(sumtemp)));
            sumtemp = vmulq_f32(d1, d);
            sum2 = vaddq_f64(sum2, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum3 = vaddq_f64(sum3, vcvt_f64_f32(vget_high_f32(sumtemp)));
        }
    }
    vst1q_f64(autoc, sum0);
    vst1q_f64(autoc + 2, sum1);
    vst1q_f64(autoc + 4, sum2);
    vst1q_f64(autoc + 6, sum3);
}

void FLAC__lpc_compute_autocorrelation_intrin_neon_lag_12(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
    // This function calculates autocorrelation with NEON
    // vector functions up to a lag of 12 (or max LPC order of 11)
    // For comments, please see FLAC__lpc_compute_autocorrelation_intrin_neon_lag_16
    int i;
    int limit = data_len - 12;
    float64x2_t sum0 = vdupq_n_f64(0.0f);
    float64x2_t sum1 = vdupq_n_f64(0.0f);
    float64x2_t sum2 = vdupq_n_f64(0.0f);
    float64x2_t sum3 = vdupq_n_f64(0.0f);
    float64x2_t sum4 = vdupq_n_f64(0.0f);
    float64x2_t sum5 = vdupq_n_f64(0.0f);
    float32x4_t sumtemp = vdupq_n_f32(0.0f);

    (void)lag;
    FLAC__ASSERT(lag <= 12);

    for (i = 0; i <= limit; i++)
    {
        float32x4_t d, d0, d1, d2;
        d0 = vld1q_f32(data + i);
        d1 = vld1q_f32(data + i + 4);
        d2 = vld1q_f32(data + i + 8);
        d = vdupq_n_f32(vgetq_lane_f32(d0, 0));

        sumtemp = vmulq_f32(d0, d);
        sum0 = vaddq_f64(sum0, vcvt_f64_f32(vget_low_f32(sumtemp)));
        sum1 = vaddq_f64(sum1, vcvt_f64_f32(vget_high_f32(sumtemp)));
        sumtemp = vmulq_f32(d1, d);
        sum2 = vaddq_f64(sum2, vcvt_f64_f32(vget_low_f32(sumtemp)));
        sum3 = vaddq_f64(sum3, vcvt_f64_f32(vget_high_f32(sumtemp)));
        sumtemp = vmulq_f32(d2, d);
        sum4 = vaddq_f64(sum4, vcvt_f64_f32(vget_low_f32(sumtemp)));
        sum5 = vaddq_f64(sum5, vcvt_f64_f32(vget_high_f32(sumtemp)));
    }

    {
        float32x4_t d0 = vdupq_n_f32(0.0f);
        float32x4_t d1 = vdupq_n_f32(0.0f);
        float32x4_t d2 = vdupq_n_f32(0.0f);
        limit++;
        if (limit < 0)
            limit = 0;

        for (i = data_len - 1; i >= limit; i--)
        {
            float32x4_t d = vdupq_n_f32(data[i]);
            d2 = shufffleVector_3012(d2);
            d1 = shufffleVector_3012(d1);
            d0 = shufffleVector_3012(d0);
            d2 = copyLane(d2, d1);
            d1 = copyLane(d1, d0);
            d0 = copyLane(d0, d);

            sumtemp = vmulq_f32(d0, d);
            sum0 = vaddq_f64(sum0, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum1 = vaddq_f64(sum1, vcvt_f64_f32(vget_high_f32(sumtemp)));
            sumtemp = vmulq_f32(d1, d);
            sum2 = vaddq_f64(sum2, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum3 = vaddq_f64(sum3, vcvt_f64_f32(vget_high_f32(sumtemp)));
            sumtemp = vmulq_f32(d2, d);
            sum4 = vaddq_f64(sum4, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum5 = vaddq_f64(sum5, vcvt_f64_f32(vget_high_f32(sumtemp)));
        }
    }
    vst1q_f64(autoc, sum0);
    vst1q_f64(autoc + 2, sum1);
    vst1q_f64(autoc + 4, sum2);
    vst1q_f64(autoc + 6, sum3);
    vst1q_f64(autoc + 8, sum4);
    vst1q_f64(autoc + 10, sum5);
}

void FLAC__lpc_compute_autocorrelation_intrin_neon_lag_16(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
    // This function calculates autocorrelation with NEON
    // vector functions up to a lag of 16 (or max LPC order of 15)
    // For comments, please see FLAC__lpc_compute_autocorrelation_intrin_neon_lag_16
    int i;
    int limit = data_len - 16;
    float64x2_t sum0 = vdupq_n_f64(0.0f);
    float64x2_t sum1 = vdupq_n_f64(0.0f);
    float64x2_t sum2 = vdupq_n_f64(0.0f);
    float64x2_t sum3 = vdupq_n_f64(0.0f);
    float64x2_t sum4 = vdupq_n_f64(0.0f);
    float64x2_t sum5 = vdupq_n_f64(0.0f);
    float64x2_t sum6 = vdupq_n_f64(0.0f);
    float64x2_t sum7 = vdupq_n_f64(0.0f);
    float32x4_t sumtemp = vdupq_n_f32(0.0f);

    (void)lag;
    FLAC__ASSERT(lag <= 16);

    // Loop forwards through samples until limit
    for (i = 0; i <= limit; i++)
    {
        float32x4_t d, d0, d1, d2, d3;
        // Load data[i..i+15] in d0..d3
        d0 = vld1q_f32(data + i);
        d1 = vld1q_f32(data + i + 4);
        d2 = vld1q_f32(data + i + 8);
        d3 = vld1q_f32(data + i + 12);
        d = vdupq_n_f32(vgetq_lane_f32(d0, 0));                         // Create vector with 4 entries data[i]
        sumtemp = vmulq_f32(d0, d);                                     // Multiply data[i] with data[i+0..3]
        sum0 = vaddq_f64(sum0, vcvt_f64_f32(vget_low_f32(sumtemp)));    // Add data[i]*data[i+0..1] (lower half of sumtemp) to sum0
        sum1 = vaddq_f64(sum1, vcvt_f64_f32(vget_high_f32(sumtemp)));   // Add data[i]*data[i+2..3] (upper half of sumtemp) to sum1
        sumtemp = vmulq_f32(d1, d);                                     // Multiply data[i] with data[i+4..7]
        sum2 = vaddq_f64(sum2, vcvt_f64_f32(vget_low_f32(sumtemp)));    // Add data[i]*data[i+5..6] (lower half of sumtemp) to sum0
        sum3 = vaddq_f64(sum3, vcvt_f64_f32(vget_high_f32(sumtemp)));   // Add data[i]*data[i+7..8] (upper half of sumtemp) to sum1
        sumtemp = vmulq_f32(d2, d);                                     // etc.
        sum4 = vaddq_f64(sum4, vcvt_f64_f32(vget_low_f32(sumtemp)));
        sum5 = vaddq_f64(sum5, vcvt_f64_f32(vget_high_f32(sumtemp)));
        sumtemp = vmulq_f32(d3, d);
        sum6 = vaddq_f64(sum6, vcvt_f64_f32(vget_low_f32(sumtemp)));
        sum7 = vaddq_f64(sum7, vcvt_f64_f32(vget_high_f32(sumtemp)));
    }

    {
        float32x4_t d0 = vdupq_n_f32(0.0f);
        float32x4_t d1 = vdupq_n_f32(0.0f);
        float32x4_t d2 = vdupq_n_f32(0.0f);
        float32x4_t d3 = vdupq_n_f32(0.0f);

        // The following makes sure we can handle situations where lag < 16
        // If the limit is negative, the loop above is skipped entirely
        limit++;
        if (limit < 0)
            limit = 0;

        // Loop backwards through samples from data_len to limit
        for (i = data_len - 1; i >= limit; i--)
        {
            float32x4_t d = vdupq_n_f32(data[i]);  // Create vector with 4 entries data[i]
            // The next 7 lines of code left-shift the elements through the 4 vectors d0..d3.
            // The 8th line adds the newly loaded element to d0. This works like a stack, where
            // data[i] is pushed onto the stack every time and the 9th element falls off
            d3 = shufffleVector_3012(d3); // Left rotate vector d3
            d2 = shufffleVector_3012(d2); // Left rotate vector d2
            d1 = shufffleVector_3012(d1); // Left rotate vector d1
            d0 = shufffleVector_3012(d0); // Left rotate vector d0

            d3 = copyLane(d3, d2);	// Copy last element of d2 to last element of d3
            d2 = copyLane(d2, d1);  // Copy last element of d1 to last element of d2
            d1 = copyLane(d1, d0);  // Copy last element of d0 to last element of d1
            d0 = copyLane(d0, d);   // Copy data[i] to last element of d0

            // The next 11 lines of code work just like in the forwards-working loop above
            sumtemp = vmulq_f32(d0, d);
            sum0 = vaddq_f64(sum0, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum1 = vaddq_f64(sum1, vcvt_f64_f32(vget_high_f32(sumtemp)));
            sumtemp = vmulq_f32(d1, d);
            sum2 = vaddq_f64(sum2, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum3 = vaddq_f64(sum3, vcvt_f64_f32(vget_high_f32(sumtemp)));
            sumtemp = vmulq_f32(d2, d);
            sum4 = vaddq_f64(sum4, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum5 = vaddq_f64(sum5, vcvt_f64_f32(vget_high_f32(sumtemp)));
            sumtemp = vmulq_f32(d3, d);
            sum6 = vaddq_f64(sum6, vcvt_f64_f32(vget_low_f32(sumtemp)));
            sum7 = vaddq_f64(sum7, vcvt_f64_f32(vget_high_f32(sumtemp)));
        }
    }

    // Store sum0..sum7 in autoc[0..16]
    vst1q_f64(autoc, sum0);
    vst1q_f64(autoc + 2, sum1);
    vst1q_f64(autoc + 4, sum2);
    vst1q_f64(autoc + 6, sum3);
    vst1q_f64(autoc + 8, sum4);
    vst1q_f64(autoc + 10, sum5);
    vst1q_f64(autoc + 12, sum6);
    vst1q_f64(autoc + 14, sum7);
}
#endif /* FLAC__CPU_ARM && FLAC__HAS_ARCH64INTRIN */
#endif /* FLAC__NO_ASM */
#endif /* FLAC__INTEGER_ONLY_LIBRARY */
