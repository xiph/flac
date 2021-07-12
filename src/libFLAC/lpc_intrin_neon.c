#include "private/cpu.h"

#ifndef FLAC__INTEGER_ONLY_LIBRARY
#ifndef FLAC__NO_ASM
#if defined FLAC__CPU_ARM && FLAC__HAS_NEONINTRIN
#include "private/lpc.h"
#include "FLAC/assert.h"
#include "FLAC/format.h"
#include <arm_neon.h>

/*
*   Vec     [1 , 2 , 3 , 4]
*   Result  [1 , 1 , 1 , 1]
*/
// Set all vector lanes to the value of lane 0 
float32x4_t shufffleVector(float32x4_t vec)
{
    float32_t lane0 = vgetq_lane_f32(vec, 0);
    vec = vsetq_lane_f32(lane0, vec, 1);
    vec = vsetq_lane_f32(lane0, vec, 2);
    vec = vsetq_lane_f32(lane0, vec, 3);

    return vec;
}
/*
*   Index   [0 , 1 , 2 , 3]
*   Mask    [3 , 0 , 1 , 2]
*   Vec     [1 , 2 , 3 , 4]
*   Result  [4 , 1 , 2 , 3]
*/
float32x4_t shufffleVector_3012(float32x4_t vec)
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
float32x4_t copyLane(float32x4_t dest, float32x4_t source)
{
    float32_t lane0 = vgetq_lane_f32(source, 0);
    dest = vsetq_lane_f32(lane0, dest, 0);
    return dest;
}

void FLAC__lpc_compute_autocorrelation_intrin_neon_lag_4(const FLAC__real data[], uint32_t data_len, uint32_t lag, FLAC__real autoc[])
{
    int i;
    int limit = data_len - 4;
    float32x4_t sum0 = vdupq_n_f32(0.0f);

    (void)lag;
    FLAC__ASSERT(lag <= 4);
    FLAC__ASSERT(lag <= data_len);

    for (i = 0; i <= limit; i++)
    {
        float32x4_t d, d0;
        d0 = vld1q_f32(data + i);
        d = shufffleVector(d0);
        sum0 = vaddq_f32(sum0, vmulq_f32(d0, d));
    }

    {
        float32x4_t d0 = vdupq_n_f32(0.0f);
        limit++;
        if (limit < 0)
            limit = 0;

        for (; i < (long)data_len; i++)
        {
            float32x4_t d = vld1q_lane_f32(data + i, vdupq_n_f32(0.0f), 0);

            d = shufffleVector(d);

            d0 = shufffleVector_3012(d0);
            d0 = copyLane(d0, d);
            sum0 = vaddq_f32(sum0, vmulq_f32(d, d0));
        }
    }
    vst1q_f32(autoc, sum0);
}

void FLAC__lpc_compute_autocorrelation_intrin_neon_lag_8(const FLAC__real data[], uint32_t data_len, uint32_t lag, FLAC__real autoc[])
{
    int i;
    int limit = data_len - 8;
    float32x4_t sum0 = vdupq_n_f32(0.0f);
    float32x4_t sum1 = vdupq_n_f32(0.0f);

    (void)lag;
    FLAC__ASSERT(lag <= 8);
    FLAC__ASSERT(lag <= data_len);

    for (i = 0; i <= limit; i++)
    {
        float32x4_t d, d0, d1;
        d0 = vld1q_f32(data + i);
        d1 = vld1q_f32(data + i + 4);
        d = shufffleVector(d0);
        sum0 = vaddq_f32(sum0, vmulq_f32(d0, d));
        sum1 = vaddq_f32(sum1, vmulq_f32(d1, d));
    }

    {
        float32x4_t d0 = vdupq_n_f32(0.0f);
        float32x4_t d1 = vdupq_n_f32(0.0f);
        limit++;
        if (limit < 0)
            limit = 0;

        for (; i < (long)data_len; i++)
        {
            float32x4_t d = vld1q_lane_f32(data + i, vdupq_n_f32(0.0f), 0);
            d = shufffleVector(d);

            d1 = shufffleVector_3012(d1);
            d0 = shufffleVector_3012(d0);

            d1 = copyLane(d1, d0);
            d0 = copyLane(d0, d);

            sum1 = vaddq_f32(sum1, vmulq_f32(d, d1));
            sum0 = vaddq_f32(sum0, vmulq_f32(d, d0));
        }
    }
    vst1q_f32(autoc, sum0);
    vst1q_f32(autoc + 4, sum1);
}

void FLAC__lpc_compute_autocorrelation_intrin_neon_lag_12(const FLAC__real data[], uint32_t data_len, uint32_t lag, FLAC__real autoc[])
{
    int i;
    int limit = data_len - 12;
    float32x4_t sum0 = vdupq_n_f32(0.0f);
    float32x4_t sum1 = vdupq_n_f32(0.0f);
    float32x4_t sum2 = vdupq_n_f32(0.0f);

    (void)lag;
    FLAC__ASSERT(lag <= 12);
    FLAC__ASSERT(lag <= data_len);

    for (i = 0; i <= limit; i++)
    {
        float32x4_t d, d0, d1, d2;
        d0 = vld1q_f32(data + i);
        d1 = vld1q_f32(data + i + 4);
        d2 = vld1q_f32(data + i + 8);
        d = shufffleVector(d0);
        sum0 = vaddq_f32(sum0, vmulq_f32(d0, d));
        sum1 = vaddq_f32(sum1, vmulq_f32(d1, d));
        sum2 = vaddq_f32(sum2, vmulq_f32(d2, d));
    }

    {
        float32x4_t d0 = vdupq_n_f32(0.0f);
        float32x4_t d1 = vdupq_n_f32(0.0f);
        float32x4_t d2 = vdupq_n_f32(0.0f);
        limit++;
        if (limit < 0)
            limit = 0;

        for (; i < (long)data_len; i++)
        {
            float32x4_t d = vld1q_lane_f32(data + i, vdupq_n_f32(0.0f), 0);
            d = shufffleVector(d);

            d2 = shufffleVector_3012(d2);
            d1 = shufffleVector_3012(d1);
            d0 = shufffleVector_3012(d0);

            d2 = copyLane(d2, d1);
            d1 = copyLane(d1, d0);
            d0 = copyLane(d0, d);

            sum2 = vaddq_f32(sum2, vmulq_f32(d, d2));
            sum1 = vaddq_f32(sum1, vmulq_f32(d, d1));
            sum0 = vaddq_f32(sum0, vmulq_f32(d, d0));
        }
    }
    vst1q_f32(autoc, sum0);
    vst1q_f32(autoc + 4, sum1);
    vst1q_f32(autoc + 8, sum2);
}

void FLAC__lpc_compute_autocorrelation_intrin_neon_lag_16(const FLAC__real data[], uint32_t data_len, uint32_t lag, FLAC__real autoc[])
{
    int i;
    int limit = data_len - 16;
    float32x4_t sum0 = vdupq_n_f32(0.0f);
    float32x4_t sum1 = vdupq_n_f32(0.0f);
    float32x4_t sum2 = vdupq_n_f32(0.0f);
    float32x4_t sum3 = vdupq_n_f32(0.0f);

    (void)lag;
    FLAC__ASSERT(lag <= 16);
    FLAC__ASSERT(lag <= data_len);

    for (i = 0; i <= limit; i++)
    {
        float32x4_t d, d0, d1, d2, d3;
        d0 = vld1q_f32(data + i);
        d1 = vld1q_f32(data + i + 4);
        d2 = vld1q_f32(data + i + 8);
        d3 = vld1q_f32(data + i + 12);
        d = shufffleVector(d0);
        sum0 = vaddq_f32(sum0, vmulq_f32(d0, d));
        sum1 = vaddq_f32(sum1, vmulq_f32(d1, d));
        sum2 = vaddq_f32(sum2, vmulq_f32(d2, d));
        sum3 = vaddq_f32(sum3, vmulq_f32(d3, d));
    }

    {
        float32x4_t d0 = vdupq_n_f32(0.0f);
        float32x4_t d1 = vdupq_n_f32(0.0f);
        float32x4_t d2 = vdupq_n_f32(0.0f);
        float32x4_t d3 = vdupq_n_f32(0.0f);
        limit++;
        if (limit < 0)
            limit = 0;

        for (; i < (long)data_len; i++)
        {
            float32x4_t d = vld1q_lane_f32(data + i, vdupq_n_f32(0.0f), 0);
            d = shufffleVector(d);

            d3 = shufffleVector_3012(d3);
            d2 = shufffleVector_3012(d2);
            d1 = shufffleVector_3012(d1);
            d0 = shufffleVector_3012(d0);

            d3 = copyLane(d3, d2);
            d2 = copyLane(d2, d1);
            d1 = copyLane(d1, d0);
            d0 = copyLane(d0, d);

            sum3 = vaddq_f32(sum3, vmulq_f32(d, d3));
            sum2 = vaddq_f32(sum2, vmulq_f32(d, d2));
            sum1 = vaddq_f32(sum1, vmulq_f32(d, d1));
            sum0 = vaddq_f32(sum0, vmulq_f32(d, d0));
        }
    }
    vst1q_f32(autoc, sum0);
    vst1q_f32(autoc + 4, sum1);
    vst1q_f32(autoc + 8, sum2);
    vst1q_f32(autoc + 12, sum3);
}
#endif /* FLAC__CPU_ARM && FLAC__HAS_ARCH64INTRIN */
#endif /* FLAC__NO_ASM */
#endif /* FLAC__INTEGER_ONLY_LIBRARY */