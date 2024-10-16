/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000-2009  Josh Coalson
 * Copyright (C) 2011-2023  Xiph.Org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "private/cpu.h"

#ifndef FLAC__INTEGER_ONLY_LIBRARY
#ifndef FLAC__NO_ASM
#ifdef FLAC__RISCV_VECTOR
#ifdef HAVE_RISCV_VECTOR_H
#if defined FLAC__CPU_RISCV64 && FLAC__HAS_RISCVINTRIN
#include "private/lpc.h"
#include "FLAC/assert.h"
#include "FLAC/format.h"
#include "private/macros.h"
#include <riscv_vector.h>

void FLAC__lpc_compute_autocorrelation_intrin_riscv(const FLAC__real data[], uint32_t data_len, uint32_t lag, double autoc[])
{
	uint32_t i;
	vfloat64m8_t sample;

	// Set LMUL=8 to group vector registers into groups of 8. Assuming a VLEN of at
	// least 128b that should yield enough space for 128x8=1024b / vector register.
	// 1024b is enough for 16 Float64s so we should be able to process the entire lag
	// window with a single virtual register.
	size_t vl = __riscv_vsetvl_e64m8(lag);

	vfloat64m8_t sums = __riscv_vfmv_v_f_f64m8(0.0, vl);
	vfloat64m8_t d = __riscv_vfmv_v_f_f64m8(0.0, vl);

	FLAC__ASSERT(vl == lag);

	for(i = 0; i < data_len; i++) {
		const double new_sample = data[i];
		sample = __riscv_vfmv_v_f_f64m8(new_sample, vl);
		d = __riscv_vfslide1up_vf_f64m8(d, new_sample, vl);
		sums = __riscv_vfmacc_vv_f64m8(sums, sample, d, vl);
	}
	__riscv_vse64_v_f64m8(autoc, sums, vl);
}

void FLAC__lpc_compute_residual_from_qlp_coefficients_intrin_riscv(const FLAC__int32 *data, uint32_t data_len, const FLAC__int32 qlp_coeff[], uint32_t order, int lp_quantization, FLAC__int32 residual[])
{
	int i;
	FLAC__int32 sum;
	vint32m1_t data_vec;
	vint32m1_t q0, q1, q2, q3, q4, q5, q6, q7, q8, q9, q10, q11;
	vint32m1_t d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11;
	vint32m1_t summ, mull;
	size_t vl = __riscv_vsetvl_e32m1(4);

	// TODO(gkalsi): Fix this here?
	FLAC__ASSERT(vl == 4);

	FLAC__ASSERT(order > 0);
	FLAC__ASSERT(order <= 32);

	if(order <= 12) {
		if(order > 8) {
			if(order > 10) {
				if(order == 12) {
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);
					q4 = __riscv_vmv_v_x_i32m1(qlp_coeff[4], vl);
					q5 = __riscv_vmv_v_x_i32m1(qlp_coeff[5], vl);
					q6 = __riscv_vmv_v_x_i32m1(qlp_coeff[6], vl);
					q7 = __riscv_vmv_v_x_i32m1(qlp_coeff[7], vl);
					q8 = __riscv_vmv_v_x_i32m1(qlp_coeff[8], vl);
					q9 = __riscv_vmv_v_x_i32m1(qlp_coeff[9], vl);
					q10 = __riscv_vmv_v_x_i32m1(qlp_coeff[10], vl);
					q11 = __riscv_vmv_v_x_i32m1(qlp_coeff[11], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);
						d4 = __riscv_vle32_v_i32m1(data + i - 5, vl);
						d5 = __riscv_vle32_v_i32m1(data + i - 6, vl);
						d6 = __riscv_vle32_v_i32m1(data + i - 7, vl);
						d7 = __riscv_vle32_v_i32m1(data + i - 8, vl);
						d8 = __riscv_vle32_v_i32m1(data + i - 9, vl);
						d9 = __riscv_vle32_v_i32m1(data + i - 10, vl);
						d10 = __riscv_vle32_v_i32m1(data + i - 11, vl);
						d11 = __riscv_vle32_v_i32m1(data + i - 12, vl);

						summ = __riscv_vmul_vv_i32m1(q11, d11, vl);
						mull = __riscv_vmul_vv_i32m1(q10, d10, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q9, d9, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q8, d8, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q7, d7, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q6, d6, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q5, d5, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q4, d4, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q3, d3, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
				else { /* order == 11 */
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);
					q4 = __riscv_vmv_v_x_i32m1(qlp_coeff[4], vl);
					q5 = __riscv_vmv_v_x_i32m1(qlp_coeff[5], vl);
					q6 = __riscv_vmv_v_x_i32m1(qlp_coeff[6], vl);
					q7 = __riscv_vmv_v_x_i32m1(qlp_coeff[7], vl);
					q8 = __riscv_vmv_v_x_i32m1(qlp_coeff[8], vl);
					q9 = __riscv_vmv_v_x_i32m1(qlp_coeff[9], vl);
					q10 = __riscv_vmv_v_x_i32m1(qlp_coeff[10], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);
						d4 = __riscv_vle32_v_i32m1(data + i - 5, vl);
						d5 = __riscv_vle32_v_i32m1(data + i - 6, vl);
						d6 = __riscv_vle32_v_i32m1(data + i - 7, vl);
						d7 = __riscv_vle32_v_i32m1(data + i - 8, vl);
						d8 = __riscv_vle32_v_i32m1(data + i - 9, vl);
						d9 = __riscv_vle32_v_i32m1(data + i - 10, vl);
						d10 = __riscv_vle32_v_i32m1(data + i - 11, vl);

						summ = __riscv_vmul_vv_i32m1(q10, d10, vl);
						mull = __riscv_vmul_vv_i32m1(q9, d9, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q8, d8, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q7, d7, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q6, d6, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q5, d5, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q4, d4, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q3, d3, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
			}
			else {
				if(order == 10) {
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);
					q4 = __riscv_vmv_v_x_i32m1(qlp_coeff[4], vl);
					q5 = __riscv_vmv_v_x_i32m1(qlp_coeff[5], vl);
					q6 = __riscv_vmv_v_x_i32m1(qlp_coeff[6], vl);
					q7 = __riscv_vmv_v_x_i32m1(qlp_coeff[7], vl);
					q8 = __riscv_vmv_v_x_i32m1(qlp_coeff[8], vl);
					q9 = __riscv_vmv_v_x_i32m1(qlp_coeff[9], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);
						d4 = __riscv_vle32_v_i32m1(data + i - 5, vl);
						d5 = __riscv_vle32_v_i32m1(data + i - 6, vl);
						d6 = __riscv_vle32_v_i32m1(data + i - 7, vl);
						d7 = __riscv_vle32_v_i32m1(data + i - 8, vl);
						d8 = __riscv_vle32_v_i32m1(data + i - 9, vl);
						d9 = __riscv_vle32_v_i32m1(data + i - 10, vl);

						summ = __riscv_vmul_vv_i32m1(q9, d9, vl);
						mull = __riscv_vmul_vv_i32m1(q8, d8, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q7, d7, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q6, d6, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q5, d5, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q4, d4, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q3, d3, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
				else { /* order == 9 */
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);
					q4 = __riscv_vmv_v_x_i32m1(qlp_coeff[4], vl);
					q5 = __riscv_vmv_v_x_i32m1(qlp_coeff[5], vl);
					q6 = __riscv_vmv_v_x_i32m1(qlp_coeff[6], vl);
					q7 = __riscv_vmv_v_x_i32m1(qlp_coeff[7], vl);
					q8 = __riscv_vmv_v_x_i32m1(qlp_coeff[8], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);
						d4 = __riscv_vle32_v_i32m1(data + i - 5, vl);
						d5 = __riscv_vle32_v_i32m1(data + i - 6, vl);
						d6 = __riscv_vle32_v_i32m1(data + i - 7, vl);
						d7 = __riscv_vle32_v_i32m1(data + i - 8, vl);
						d8 = __riscv_vle32_v_i32m1(data + i - 9, vl);

						summ = __riscv_vmul_vv_i32m1(q8, d8, vl);
						mull = __riscv_vmul_vv_i32m1(q7, d7, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q6, d6, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q5, d5, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q4, d4, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q3, d3, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
			}
		}
		else if(order > 4) {
			if(order > 6) {
				if(order == 8) {
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);
					q4 = __riscv_vmv_v_x_i32m1(qlp_coeff[4], vl);
					q5 = __riscv_vmv_v_x_i32m1(qlp_coeff[5], vl);
					q6 = __riscv_vmv_v_x_i32m1(qlp_coeff[6], vl);
					q7 = __riscv_vmv_v_x_i32m1(qlp_coeff[7], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);
						d4 = __riscv_vle32_v_i32m1(data + i - 5, vl);
						d5 = __riscv_vle32_v_i32m1(data + i - 6, vl);
						d6 = __riscv_vle32_v_i32m1(data + i - 7, vl);
						d7 = __riscv_vle32_v_i32m1(data + i - 8, vl);

						summ = __riscv_vmul_vv_i32m1(q7, d7, vl);
						mull = __riscv_vmul_vv_i32m1(q6, d6, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q5, d5, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q4, d4, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q3, d3, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
				else { /* order == 7 */
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);
					q4 = __riscv_vmv_v_x_i32m1(qlp_coeff[4], vl);
					q5 = __riscv_vmv_v_x_i32m1(qlp_coeff[5], vl);
					q6 = __riscv_vmv_v_x_i32m1(qlp_coeff[6], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);
						d4 = __riscv_vle32_v_i32m1(data + i - 5, vl);
						d5 = __riscv_vle32_v_i32m1(data + i - 6, vl);
						d6 = __riscv_vle32_v_i32m1(data + i - 7, vl);
						summ = __riscv_vmul_vv_i32m1(q6, d6, vl);
						mull = __riscv_vmul_vv_i32m1(q5, d5, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q4, d4, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q3, d3, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
			}
			else {
				if(order == 6) {
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);
					q4 = __riscv_vmv_v_x_i32m1(qlp_coeff[4], vl);
					q5 = __riscv_vmv_v_x_i32m1(qlp_coeff[5], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);
						d4 = __riscv_vle32_v_i32m1(data + i - 5, vl);
						d5 = __riscv_vle32_v_i32m1(data + i - 6, vl);

						summ = __riscv_vmul_vv_i32m1(q5, d5, vl);
						mull = __riscv_vmul_vv_i32m1(q4, d4, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q3, d3, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
				else { /* order == 5 */
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);
					q4 = __riscv_vmv_v_x_i32m1(qlp_coeff[4], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);
						d4 = __riscv_vle32_v_i32m1(data + i - 5, vl);

						summ = __riscv_vmul_vv_i32m1(q4, d4, vl);
						mull = __riscv_vmul_vv_i32m1(q3, d3, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
			}
		}
		else {
			if(order > 2) {
				if(order == 4) {
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);
					q3 = __riscv_vmv_v_x_i32m1(qlp_coeff[3], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);
						d3 = __riscv_vle32_v_i32m1(data + i - 4, vl);

						summ = __riscv_vmul_vv_i32m1(q3, d3, vl);
						mull = __riscv_vmul_vv_i32m1(q2, d2, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
				else { /* order == 3 */
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);
					q2 = __riscv_vmv_v_x_i32m1(qlp_coeff[2], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);
						d2 = __riscv_vle32_v_i32m1(data + i - 3, vl);

						summ = __riscv_vmul_vv_i32m1(q2, d2, vl);
						mull = __riscv_vmul_vv_i32m1(q1, d1, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
			}
			else {
				if(order == 2) {
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);
					q1 = __riscv_vmv_v_x_i32m1(qlp_coeff[1], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);
						d1 = __riscv_vle32_v_i32m1(data + i - 2, vl);

						summ = __riscv_vmul_vv_i32m1(q1, d1, vl);
						mull = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vadd_vv_i32m1(summ, mull, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
				else { /* order == 1 */
					q0 = __riscv_vmv_v_x_i32m1(qlp_coeff[0], vl);

					for(i = 0; i < (int)data_len - 3; i += 4) {
						d0 = __riscv_vle32_v_i32m1(data + i - 1, vl);

						summ = __riscv_vmul_vv_i32m1(q0, d0, vl);
						summ = __riscv_vsra_vx_i32m1(summ, lp_quantization, vl);

						data_vec = __riscv_vle32_v_i32m1(data + i, vl);
						data_vec = __riscv_vsub_vv_i32m1(data_vec, summ, vl);
						__riscv_vse32_v_i32m1(residual + i, data_vec, vl);
					}
				}
			}
		}
		for(; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 12:
					sum += qlp_coeff[11] * data[i - 12]; /* Falls through. */
				case 11:
					sum += qlp_coeff[10] * data[i - 11]; /* Falls through. */
				case 10:
					sum += qlp_coeff[9] * data[i - 10]; /* Falls through. */
				case 9:
					sum += qlp_coeff[8] * data[i - 9]; /* Falls through. */
				case 8:
					sum += qlp_coeff[7] * data[i - 8]; /* Falls through. */
				case 7:
					sum += qlp_coeff[6] * data[i - 7]; /* Falls through. */
				case 6:
					sum += qlp_coeff[5] * data[i - 6]; /* Falls through. */
				case 5:
					sum += qlp_coeff[4] * data[i - 5]; /* Falls through. */
				case 4:
					sum += qlp_coeff[3] * data[i - 4]; /* Falls through. */
				case 3:
					sum += qlp_coeff[2] * data[i - 3]; /* Falls through. */
				case 2:
					sum += qlp_coeff[1] * data[i - 2]; /* Falls through. */
				case 1:
					sum += qlp_coeff[0] * data[i - 1];
			}
			residual[i] = data[i] - (sum >> lp_quantization);
		}
	}
	else { /* order > 12 */
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 32:
					sum += qlp_coeff[31] * data[i - 32]; /* Falls through. */
				case 31:
					sum += qlp_coeff[30] * data[i - 31]; /* Falls through. */
				case 30:
					sum += qlp_coeff[29] * data[i - 30]; /* Falls through. */
				case 29:
					sum += qlp_coeff[28] * data[i - 29]; /* Falls through. */
				case 28:
					sum += qlp_coeff[27] * data[i - 28]; /* Falls through. */
				case 27:
					sum += qlp_coeff[26] * data[i - 27]; /* Falls through. */
				case 26:
					sum += qlp_coeff[25] * data[i - 26]; /* Falls through. */
				case 25:
					sum += qlp_coeff[24] * data[i - 25]; /* Falls through. */
				case 24:
					sum += qlp_coeff[23] * data[i - 24]; /* Falls through. */
				case 23:
					sum += qlp_coeff[22] * data[i - 23]; /* Falls through. */
				case 22:
					sum += qlp_coeff[21] * data[i - 22]; /* Falls through. */
				case 21:
					sum += qlp_coeff[20] * data[i - 21]; /* Falls through. */
				case 20:
					sum += qlp_coeff[19] * data[i - 20]; /* Falls through. */
				case 19:
					sum += qlp_coeff[18] * data[i - 19]; /* Falls through. */
				case 18:
					sum += qlp_coeff[17] * data[i - 18]; /* Falls through. */
				case 17:
					sum += qlp_coeff[16] * data[i - 17]; /* Falls through. */
				case 16:
					sum += qlp_coeff[15] * data[i - 16]; /* Falls through. */
				case 15:
					sum += qlp_coeff[14] * data[i - 15]; /* Falls through. */
				case 14:
					sum += qlp_coeff[13] * data[i - 14]; /* Falls through. */
				case 13:
					sum += qlp_coeff[12] * data[i - 13];
					sum += qlp_coeff[11] * data[i - 12];
					sum += qlp_coeff[10] * data[i - 11];
					sum += qlp_coeff[9] * data[i - 10];
					sum += qlp_coeff[8] * data[i - 9];
					sum += qlp_coeff[7] * data[i - 8];
					sum += qlp_coeff[6] * data[i - 7];
					sum += qlp_coeff[5] * data[i - 6];
					sum += qlp_coeff[4] * data[i - 5];
					sum += qlp_coeff[3] * data[i - 4];
					sum += qlp_coeff[2] * data[i - 3];
					sum += qlp_coeff[1] * data[i - 2];
					sum += qlp_coeff[0] * data[i - 1];
			}
			residual[i] = data[i] - (sum >> lp_quantization);
		}
	}
}

#endif /* FLAC__CPU_ARM64 && FLAC__HAS_ARCH64INTRIN */
#endif /* HAVE_RISCV_VECTOR_H */
#endif /* FLAC__RISCV_VECTOR */
#endif /* FLAC__NO_ASM */
#endif /* FLAC__INTEGER_ONLY_LIBRARY */
