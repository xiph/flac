; libFLAC - Free Lossless Audio Codec library
; Copyright (C) 2001  Josh Coalson
;
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Library General Public
; License as published by the Free Software Foundation; either
; version 2 of the License, or (at your option) any later version.
;
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Library General Public License for more details.
;
; You should have received a copy of the GNU Library General Public
; License along with this library; if not, write to the
; Free Software Foundation, Inc., 59 Temple Place - Suite 330,
; Boston, MA  02111-1307, USA.

%include "nasm.h"

	data_section

cglobal FLAC__lpc_compute_autocorrelation_asm_i386
cglobal FLAC__lpc_compute_autocorrelation_asm_i386_sse
cglobal FLAC__lpc_compute_residual_from_qlp_coefficients_asm_i386
cglobal FLAC__lpc_compute_residual_from_qlp_coefficients_asm_i386_mmx
cglobal FLAC__lpc_restore_signal_asm_i386
cglobal FLAC__lpc_restore_signal_asm_i386_mmx

	code_section

; **********************************************************************
;
; void FLAC__lpc_compute_autocorrelation_asm(const real data[], unsigned data_len, unsigned lag, real autoc[])
; {
;	real d;
;	unsigned sample, coeff;
;	const unsigned limit = data_len - lag;
;
;	assert(lag > 0);
;	assert(lag <= data_len);
;
;	for(coeff = 0; coeff < lag; coeff++)
;		autoc[coeff] = 0.0;
;	for(sample = 0; sample <= limit; sample++) {
;		d = data[sample];
;		for(coeff = 0; coeff < lag; coeff++)
;			autoc[coeff] += d * data[sample+coeff];
;	}
;	for(; sample < data_len; sample++) {
;		d = data[sample];
;		for(coeff = 0; coeff < data_len - sample; coeff++)
;			autoc[coeff] += d * data[sample+coeff];
;	}
; }
;
	ALIGN 16
cident FLAC__lpc_compute_autocorrelation_asm_i386:

	; esp + 32 == autoc[]
	; esp + 28 == lag
	; esp + 24 == data_len
	; esp + 20 == data[]

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	edx, [esp + 28]			; edx == lag
	mov	edi, [esp + 32]			; edi == autoc

	;	for(coeff = 0; coeff < lag; coeff++)
	;		autoc[coeff] = 0.0;
	mov	ecx, edx			; ecx = # of dwords of 0 to write
	xor	eax, eax
	rep	stosd

	;	const unsigned limit = data_len - lag;
	mov	esi, [esp + 24]			; esi == data_len
	mov	ecx, esi
	sub	ecx, edx			; ecx == limit
	mov	edi, [esp + 32]			; edi == autoc
	inc	ecx				; we are looping <= limit so we add one to the counter

	;	for(sample = 0; sample <= limit; sample++) {
	;		d = data[sample];
	;		for(coeff = 0; coeff < lag; coeff++)
	;			autoc[coeff] += d * data[sample+coeff];
	;	}
	mov	eax, [esp + 20]			; eax == &data[sample] <- &data[0]
	xor	ebp, ebp			; ebp == sample <- 0
	ALIGN 16
.outer_loop:
	push	eax				; save &data[sample], for inner_loop:
						; eax == &data[sample+coeff] <- &data[sample]
	fld	dword [eax]			; ST = d <- data[sample]
	mov	edx, [esp + 32]			; edx <- lag (note the +4 due to the above 'push eax')
	mov	ebx, edi			; ebx == &autoc[coeff] <- &autoc[0]
	and	edx, 3				; edx <- lag % 4
	jz	.inner_start
	cmp	edx, 1
	je	.warmup_1
	cmp	edx, 2
	je	.warmup_2
.warmup_3:
	fld	st0				; ST = d d
	fmul	dword [eax]			; ST = d*data[sample+coeff] d
	add	eax, byte 4			; (sample+coeff)++
	fadd	dword [ebx]			; ST = autoc[coeff]+d*data[sample+coeff] d
	fstp	dword [ebx]			; autoc[coeff]+=d*data[sample+coeff]  ST = d
	add	ebx, byte 4			; coeff++
.warmup_2:
	fld	st0				; ST = d d
	fmul	dword [eax]			; ST = d*data[sample+coeff] d
	add	eax, byte 4			; (sample+coeff)++
	fadd	dword [ebx]			; ST = autoc[coeff]+d*data[sample+coeff] d
	fstp	dword [ebx]			; autoc[coeff]+=d*data[sample+coeff]  ST = d
	add	ebx, byte 4			; coeff++
.warmup_1:
	fld	st0				; ST = d d
	fmul	dword [eax]			; ST = d*data[sample+coeff] d
	add	eax, byte 4			; (sample+coeff)++
	fadd	dword [ebx]			; ST = autoc[coeff]+d*data[sample+coeff] d
	fstp	dword [ebx]			; autoc[coeff]+=d*data[sample+coeff]  ST = d
	add	ebx, byte 4			; coeff++
.inner_start:
	mov	edx, [esp + 32]			; edx <- lag (note the +4 due to the above 'push eax')
	shr	edx, 2				; edx <- lag / 4
	jz	.inner_end
	ALIGN 16
.inner_loop:
	fld	st0				; ST = d d
	fmul	dword [eax]			; ST = d*data[sample+coeff] d
	add	eax, byte 4			; (sample+coeff)++
	fadd	dword [ebx]			; ST = autoc[coeff]+d*data[sample+coeff] d
	fstp	dword [ebx]			; autoc[coeff]+=d*data[sample+coeff]  ST = d
	add	ebx, byte 4			; coeff++
	fld	st0				; ST = d d
	fmul	dword [eax]			; ST = d*data[sample+coeff] d
	add	eax, byte 4			; (sample+coeff)++
	fadd	dword [ebx]			; ST = autoc[coeff]+d*data[sample+coeff] d
	fstp	dword [ebx]			; autoc[coeff]+=d*data[sample+coeff]  ST = d
	add	ebx, byte 4			; coeff++
	fld	st0				; ST = d d
	fmul	dword [eax]			; ST = d*data[sample+coeff] d
	add	eax, byte 4			; (sample+coeff)++
	fadd	dword [ebx]			; ST = autoc[coeff]+d*data[sample+coeff] d
	fstp	dword [ebx]			; autoc[coeff]+=d*data[sample+coeff]  ST = d
	add	ebx, byte 4			; coeff++
	fld	st0				; ST = d d
	fmul	dword [eax]			; ST = d*data[sample+coeff] d
	add	eax, byte 4			; (sample+coeff)++
	fadd	dword [ebx]			; ST = autoc[coeff]+d*data[sample+coeff] d
	fstp	dword [ebx]			; autoc[coeff]+=d*data[sample+coeff]  ST = d
	add	ebx, byte 4			; coeff++
	dec	edx
	jnz	.inner_loop
.inner_end:
	pop	eax				; restore &data[sample]
	fstp	st0				; pop d, ST = empty
	inc	ebp				; sample++
	add	eax, byte 4			; &data[sample++]
	dec	ecx
	jnz	near .outer_loop

	;	for(; sample < data_len; sample++) {
	;		d = data[sample];
	;		for(coeff = 0; coeff < data_len - sample; coeff++)
	;			autoc[coeff] += d * data[sample+coeff];
	;	}
	mov	ecx, [esp + 28]			; ecx <- lag
	dec	ecx				; ecx <- lag - 1
	jz	.end				; skip loop if 0
	ALIGN 16
.outer_loop2:
	push	eax				; save &data[sample], for inner_loop2:
						; eax == &data[sample+coeff] <- &data[sample]
	fld	dword [eax]			; ST = d <- data[sample]
	mov	edx, esi			; edx <- data_len
	sub	edx, ebp			; edx <- data_len-sample
	mov	ebx, edi			; ebx == &autoc[coeff] <- &autoc[0]
	ALIGN 16
.inner_loop2:
	fld	st0				; ST = d d
	fmul	dword [eax]			; ST = d*data[sample+coeff] d
	add	eax, byte 4			; (sample+coeff)++
	fadd	dword [ebx]			; ST = autoc[coeff]+d*data[sample+coeff] d
	fstp	dword [ebx]			; autoc[coeff]+=d*data[sample+coeff]  ST = d
	add	ebx, byte 4			; coeff++
	dec	edx
	jnz	.inner_loop2
	pop	eax				; restore &data[sample]
	fstp	st0				; pop d, ST = empty
	inc	ebp				; sample++
	add	eax, byte 4			; &data[sample++]
	dec	ecx
	jnz	.outer_loop2

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

;@@@ NOTE: this SSE version is not even tested yet and only works for lag == 8
	ALIGN 16
cident FLAC__lpc_compute_autocorrelation_asm_i386_sse:

	; esp + 16 == autoc[]
	; esp + 12 == lag
	; esp + 8 == data_len
	; esp + 4 == data[]

	;	for(coeff = 0; coeff < lag; coeff++)
	;		autoc[coeff] = 0.0;
	xorps	xmm6, xmm6
	xorps	xmm7, xmm7

	mov	edx, [esp + 8]			; edx == data_len
	mov	eax, [esp + 4]			; eax == &data[sample] <- &data[0]

	movss	xmm0, [eax]			; xmm0 = 0,0,0,data[0]
	add	eax, 4
	movaps	xmm2, xmm0			; xmm2 = 0,0,0,data[0]
	shufps	xmm0, xmm0, 0			; xmm0 = data[0],data[0],data[0],data[0]
	movaps	xmm1, xmm0			; xmm1 = data[0],data[0],data[0],data[0]
	xorps	xmm3, xmm3			; xmm3 = 0,0,0,0
.warmup:					; xmm3:xmm2 = data[sample-[7..0]]
	movaps	xmm4, xmm0
	movaps	xmm5, xmm1			; xmm5:xmm4 = xmm1:xmm0 = data[sample]*8
	mulps	xmm4, xmm2
	mulps	xmm5, xmm3			; xmm5:xmm4 = xmm1:xmm0 * xmm3:xmm2
	addps	xmm6, xmm4
	addps	xmm7, xmm5			; xmm7:xmm6 += xmm1:xmm0 * xmm3:xmm2
	dec	edx
	;* there's no need to even check for this because we know that lag == 8
	;* and data_len >= lag, so our 1-sample warmup cannot finish the loop
	; jz	.loop_end
	ALIGN 16
.loop_8:
	; read the next sample
	movss	xmm0, [eax]			; xmm0 = 0,0,0,data[sample]
	add	eax, 4
	shufps	xmm0, xmm0, 0			; xmm0 = data[sample],data[sample],data[sample],data[sample]
	movaps	xmm1, xmm0			; xmm1 = data[sample],data[sample],data[sample],data[sample]
	; now shift the lagged samples
	movaps	xmm4, xmm2
	movaps	xmm5, xmm3
	shufps	xmm2, xmm4, 93h			; 93h=2-1-0-3 => xmm2 gets rotated left by one float
	shufps	xmm3, xmm5, 93h			; 93h=2-1-0-3 => xmm3 gets rotated left by one float
	movss	xmm3, xmm2
	movss	xmm2, xmm0

	movaps	xmm4, xmm0
	movaps	xmm5, xmm1			; xmm5:xmm4 = xmm1:xmm0 = data[sample]*8
	mulps	xmm4, xmm2
	mulps	xmm5, xmm3			; xmm5:xmm4 = xmm1:xmm0 * xmm3:xmm2
	addps	xmm6, xmm4
	addps	xmm7, xmm5			; xmm7:xmm6 += xmm1:xmm0 * xmm3:xmm2
	dec	edx
	jnz	.loop_8
.loop_end:
	; store autoc
	mov	edx, [esp + 16]			; edx == autoc
	movups	xmm6, [edx]
	movups	xmm7, [edx + 4]

.end:
	ret

;void FLAC__lpc_compute_residual_from_qlp_coefficients(const int32 data[], unsigned data_len, const int32 qlp_coeff[], unsigned order, int lp_quantization, int32 residual[])
;
;	for(i = 0; i < data_len; i++) {
;		sum = 0;
;		for(j = 0; j < order; j++)
;			sum += qlp_coeff[j] * data[i-j-1];
;		residual[i] = data[i] - (sum >> lp_quantization);
;	}
;
	ALIGN	16
cident FLAC__lpc_compute_residual_from_qlp_coefficients_asm_i386:
	;[esp + 40]	residual[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	data[]

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	esi, [esp + 20]			; esi = data[]
	mov	edi, [esp + 40]			; edi = residual[]
	mov	eax, [esp + 32]			; eax = order
	mov	ebx, [esp + 24]			; ebx = data_len

	test	ebx, ebx
	jz	.end				; do nothing if data_len == 0
.begin:
	cmp	eax, byte 1
	jg	short .i_1more

	mov	ecx, [esp + 28]
	mov	edx, [ecx]			; edx = qlp_coeff[0]
	mov	eax, [esi - 4]			; eax = data[-1]
	mov	cl, [esp + 36]			; cl = lp_quantization
	ALIGN	16
.i_1_loop_i:
	imul	eax, edx
	sar	eax, cl
	neg	eax
	add	eax, [esi]
	mov	[edi], eax
	mov	eax, [esi]
	add	edi, byte 4
	add	esi, byte 4
	dec	ebx
	jnz	.i_1_loop_i

	jmp	.end

.i_1more:
	cmp	eax, byte 32			; for order <= 32 there is a faster routine
	jbe	short .i_32

	; This version is here just for completeness, since FLAC__MAX_LPC_ORDER == 32
	ALIGN 16
.i_32more_loop_i:
	xor	ebp, ebp
	mov	ecx, [esp + 32]
	mov	edx, ecx
	shl	edx, 2
	add	edx, [esp + 28]
	neg	ecx
	ALIGN	16
.i_32more_loop_j:
	sub	edx, byte 4
	mov	eax, [edx]
	imul	eax, [esi + 4 * ecx]
	add	ebp, eax
	inc	ecx
	jnz	short .i_32more_loop_j

	mov	cl, [esp + 36]
	sar	ebp, cl
	neg	ebp
	add	ebp, [esi]
	mov	[edi], ebp
	add	esi, byte 4
	add	edi, byte 4

	dec	ebx
	jnz	.i_32more_loop_i

	jmp	.end

.i_32:
	sub	edi, esi
	neg	eax
	lea	edx, [eax + eax * 8 + .jumper_0]
	inc	edx
	mov	eax, [esp + 28]			; eax = qlp_coeff[]
	xor	ebp, ebp
	jmp	edx

	mov	ecx, [eax + 124]
	imul	ecx, [esi - 128]
	add	ebp, ecx
	mov	ecx, [eax + 120]
	imul	ecx, [esi - 124]
	add	ebp, ecx
	mov	ecx, [eax + 116]
	imul	ecx, [esi - 120]
	add	ebp, ecx
	mov	ecx, [eax + 112]
	imul	ecx, [esi - 116]
	add	ebp, ecx
	mov	ecx, [eax + 108]
	imul	ecx, [esi - 112]
	add	ebp, ecx
	mov	ecx, [eax + 104]
	imul	ecx, [esi - 108]
	add	ebp, ecx
	mov	ecx, [eax + 100]
	imul	ecx, [esi - 104]
	add	ebp, ecx
	mov	ecx, [eax + 96]
	imul	ecx, [esi - 100]
	add	ebp, ecx
	mov	ecx, [eax + 92]
	imul	ecx, [esi - 96]
	add	ebp, ecx
	mov	ecx, [eax + 88]
	imul	ecx, [esi - 92]
	add	ebp, ecx
	mov	ecx, [eax + 84]
	imul	ecx, [esi - 88]
	add	ebp, ecx
	mov	ecx, [eax + 80]
	imul	ecx, [esi - 84]
	add	ebp, ecx
	mov	ecx, [eax + 76]
	imul	ecx, [esi - 80]
	add	ebp, ecx
	mov	ecx, [eax + 72]
	imul	ecx, [esi - 76]
	add	ebp, ecx
	mov	ecx, [eax + 68]
	imul	ecx, [esi - 72]
	add	ebp, ecx
	mov	ecx, [eax + 64]
	imul	ecx, [esi - 68]
	add	ebp, ecx
	mov	ecx, [eax + 60]
	imul	ecx, [esi - 64]
	add	ebp, ecx
	mov	ecx, [eax + 56]
	imul	ecx, [esi - 60]
	add	ebp, ecx
	mov	ecx, [eax + 52]
	imul	ecx, [esi - 56]
	add	ebp, ecx
	mov	ecx, [eax + 48]
	imul	ecx, [esi - 52]
	add	ebp, ecx
	mov	ecx, [eax + 44]
	imul	ecx, [esi - 48]
	add	ebp, ecx
	mov	ecx, [eax + 40]
	imul	ecx, [esi - 44]
	add	ebp, ecx
	mov	ecx, [eax + 36]
	imul	ecx, [esi - 40]
	add	ebp, ecx
	mov	ecx, [eax + 32]
	imul	ecx, [esi - 36]
	add	ebp, ecx
	mov	ecx, [eax + 28]
	imul	ecx, [esi - 32]
	add	ebp, ecx
	mov	ecx, [eax + 24]
	imul	ecx, [esi - 28]
	add	ebp, ecx
	mov	ecx, [eax + 20]
	imul	ecx, [esi - 24]
	add	ebp, ecx
	mov	ecx, [eax + 16]
	imul	ecx, [esi - 20]
	add	ebp, ecx
	mov	ecx, [eax + 12]
	imul	ecx, [esi - 16]
	add	ebp, ecx
	mov	ecx, [eax + 8]
	imul	ecx, [esi - 12]
	add	ebp, ecx
	mov	ecx, [eax + 4]
	imul	ecx, [esi - 8]
	add	ebp, ecx
	mov	ecx, [eax]		;there is one byte missing
	imul	ecx, [esi - 4]
	add	ebp, ecx
.jumper_0:

	mov	cl, [esp + 36]
	sar	ebp, cl
	neg	ebp
	add	ebp, [esi]
	mov	[edi + esi], ebp
	add	esi, byte 4

	dec	ebx
	jz	short .end
	xor	ebp, ebp
	jmp	edx

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

; WATCHOUT: this routine works on 16 bit data which means bits-per-sample for
; the channel must be <= 16.  Especially note that this routine cannot be used
; for side-channel coded 16bps channels since the effective bps is 17.
	ALIGN	16
cident FLAC__lpc_compute_residual_from_qlp_coefficients_asm_i386_mmx:
	;[esp + 40]	residual[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	data[]

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	esi, [esp + 20]			; esi = data[]
	mov	edi, [esp + 40]			; edi = residual[]
	mov	eax, [esp + 32]			; eax = order
	mov	ebx, [esp + 24]			; ebx = data_len

	test	ebx, ebx
	jz	near .end			; do nothing if data_len == 0
	dec	ebx
	test	ebx, ebx
	jz	near .last_one

	mov	edx, [esp + 28]			; edx = qlp_coeff[]
	movd	mm6, [esp + 36]			; mm6 = 0:lp_quantization
	mov	ebp, esp

	and	esp, 0xfffffff8

	xor	ecx, ecx
.copy_qlp_loop:
	push	word [edx + 4 * ecx]
	inc	ecx
	cmp	ecx, eax
	jnz	short .copy_qlp_loop

	and	ecx, 0x3
	test	ecx, ecx
	je	short .za_end
	sub	ecx, byte 4
.za_loop:
	push	word 0
	inc	eax
	inc	ecx
	jnz	short .za_loop
.za_end:

	movq	mm5, [esp + 2 * eax - 8]
	movd	mm4, [esi - 16]
	punpckldq	mm4, [esi - 12]
	movd	mm0, [esi - 8]
	punpckldq	mm0, [esi - 4]
	packssdw	mm4, mm0

	cmp	eax, byte 4
	jnbe	short .mmx_4more

	align	16
.mmx_4_loop_i:
	movd	mm1, [esi]
	movq	mm3, mm4
	punpckldq	mm1, [esi + 4]
	psrlq	mm4, 16
	movq	mm0, mm1
	psllq	mm0, 48
	por	mm4, mm0
	movq	mm2, mm4
	psrlq	mm4, 16
	pxor	mm0, mm0
	punpckhdq	mm0, mm1
	pmaddwd	mm3, mm5
	pmaddwd	mm2, mm5
	psllq	mm0, 16
	por	mm4, mm0
	movq	mm0, mm3
	punpckldq	mm3, mm2
	punpckhdq	mm0, mm2
	paddd	mm3, mm0
	psrad	mm3, mm6
	psubd	mm1, mm3
	movd	[edi], mm1
	punpckhdq	mm1, mm1
	movd	[edi + 4], mm1

	add	edi, byte 8
	add	esi, byte 8

	sub	ebx, 2
	jg	.mmx_4_loop_i
	jmp	.mmx_end
	
.mmx_4more:
	shl	eax, 2
	neg	eax
	add	eax, byte 16

	align	16
.mmx_4more_loop_i:
	movd	mm1, [esi]
	punpckldq	mm1, [esi + 4]
	movq	mm3, mm4
	psrlq	mm4, 16
	movq	mm0, mm1
	psllq	mm0, 48
	por	mm4, mm0
	movq	mm2, mm4
	psrlq	mm4, 16
	pxor	mm0, mm0
	punpckhdq	mm0, mm1
	pmaddwd	mm3, mm5
	pmaddwd	mm2, mm5
	psllq	mm0, 16
	por	mm4, mm0

	mov	ecx, esi
	add	ecx, eax
	mov	edx, esp

	align	16
.mmx_4more_loop_j:
	movd	mm0, [ecx - 16]
	movd	mm7, [ecx - 8]
	punpckldq	mm0, [ecx - 12]
	punpckldq	mm7, [ecx - 4]
	packssdw	mm0, mm7
	pmaddwd	mm0, [edx]
	punpckhdq	mm7, mm7
	paddd	mm3, mm0
	movd	mm0, [ecx - 12]
	punpckldq	mm0, [ecx - 8]
	punpckldq	mm7, [ecx]
	packssdw	mm0, mm7
	pmaddwd	mm0, [edx]
	paddd	mm2, mm0

	add	edx, byte 8
	add	ecx, byte 16
	cmp	ecx, esi
	jnz	.mmx_4more_loop_j

	movq	mm0, mm3
	punpckldq	mm3, mm2
	punpckhdq	mm0, mm2
	paddd	mm3, mm0
	psrad	mm3, mm6
	psubd	mm1, mm3
	movd	[edi], mm1
	punpckhdq	mm1, mm1
	movd	[edi + 4], mm1

	add	edi, byte 8
	add	esi, byte 8

	sub	ebx, 2
	jg	near .mmx_4more_loop_i

.mmx_end:
	emms
	mov	esp, ebp
.last_one:
	mov	eax, [esp + 32]
	inc	ebx
	jnz	near FLAC__lpc_compute_residual_from_qlp_coefficients_asm_i386.begin

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

; **********************************************************************
;
; void FLAC__lpc_restore_signal(const int32 residual[], unsigned data_len, const int32 qlp_coeff[], unsigned order, int lp_quantization, int32 data[])
; {
; 	unsigned i, j;
; 	int32 sum;
;
; 	assert(order > 0);
;
; 	for(i = 0; i < data_len; i++) {
; 		sum = 0;
; 		for(j = 0; j < order; j++)
; 			sum += qlp_coeff[j] * data[i-j-1];
; 		data[i] = residual[i] + (sum >> lp_quantization);
; 	}
; }
	ALIGN	16
cident FLAC__lpc_restore_signal_asm_i386:
	;[esp + 40]	data[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	residual[]

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	esi, [esp + 20]			; esi = residual[]
	mov	edi, [esp + 40]			; edi = data[]
	mov	eax, [esp + 32]			; eax = order
	mov	ebx, [esp + 24]			; ebx = data_len

	test	ebx, ebx
	jz	.end				; do nothing if data_len == 0

	cmp	eax, byte 1
	jg	short .x87_1more

	mov	ecx, [esp + 28]
	mov	edx, [ecx]
	mov	eax, [edi - 4]
	mov	cl, [esp + 36]
	ALIGN	16
.x87_1_loop_i:
	imul	eax, edx
	sar	eax, cl
	add	eax, [esi]
	mov	[edi], eax
	add	esi, byte 4
	add	edi, byte 4
	dec	ebx
	jnz	.x87_1_loop_i

	jmp	.end

.x87_1more:
	cmp	eax, byte 32			; for order <= 32 there is a faster routine
	jbe	short .x87_32

	; This version is here just for completeness, since FLAC__MAX_LPC_ORDER == 32
	ALIGN 16
.x87_32more_loop_i:
	xor	ebp, ebp
	mov	ecx, [esp + 32]
	mov	edx, ecx
	shl	edx, 2
	add	edx, [esp + 28]
	neg	ecx
	ALIGN	16
.x87_32more_loop_j:
	sub	edx, byte 4
	mov	eax, [edx]
	imul	eax, [edi + 4 * ecx]
	add	ebp, eax
	inc	ecx
	jnz	short .x87_32more_loop_j

	mov	cl, [esp + 36]
	sar	ebp, cl
	add	ebp, [esi]
	mov	[edi], ebp
	add	edi, byte 4
	add	esi, byte 4

	dec	ebx
	jnz	.x87_32more_loop_i

	jmp	.end

.x87_32:
	sub	esi, edi
	neg	eax
	lea	edx, [eax + eax * 8 + .jumper_0]
	inc	edx
	mov	eax, [esp + 28]
	xor	ebp, ebp
	jmp	edx

	mov	ecx, [eax + 124]
	imul	ecx, [edi - 128]
	add	ebp, ecx
	mov	ecx, [eax + 120]
	imul	ecx, [edi - 124]
	add	ebp, ecx
	mov	ecx, [eax + 116]
	imul	ecx, [edi - 120]
	add	ebp, ecx
	mov	ecx, [eax + 112]
	imul	ecx, [edi - 116]
	add	ebp, ecx
	mov	ecx, [eax + 108]
	imul	ecx, [edi - 112]
	add	ebp, ecx
	mov	ecx, [eax + 104]
	imul	ecx, [edi - 108]
	add	ebp, ecx
	mov	ecx, [eax + 100]
	imul	ecx, [edi - 104]
	add	ebp, ecx
	mov	ecx, [eax + 96]
	imul	ecx, [edi - 100]
	add	ebp, ecx
	mov	ecx, [eax + 92]
	imul	ecx, [edi - 96]
	add	ebp, ecx
	mov	ecx, [eax + 88]
	imul	ecx, [edi - 92]
	add	ebp, ecx
	mov	ecx, [eax + 84]
	imul	ecx, [edi - 88]
	add	ebp, ecx
	mov	ecx, [eax + 80]
	imul	ecx, [edi - 84]
	add	ebp, ecx
	mov	ecx, [eax + 76]
	imul	ecx, [edi - 80]
	add	ebp, ecx
	mov	ecx, [eax + 72]
	imul	ecx, [edi - 76]
	add	ebp, ecx
	mov	ecx, [eax + 68]
	imul	ecx, [edi - 72]
	add	ebp, ecx
	mov	ecx, [eax + 64]
	imul	ecx, [edi - 68]
	add	ebp, ecx
	mov	ecx, [eax + 60]
	imul	ecx, [edi - 64]
	add	ebp, ecx
	mov	ecx, [eax + 56]
	imul	ecx, [edi - 60]
	add	ebp, ecx
	mov	ecx, [eax + 52]
	imul	ecx, [edi - 56]
	add	ebp, ecx
	mov	ecx, [eax + 48]
	imul	ecx, [edi - 52]
	add	ebp, ecx
	mov	ecx, [eax + 44]
	imul	ecx, [edi - 48]
	add	ebp, ecx
	mov	ecx, [eax + 40]
	imul	ecx, [edi - 44]
	add	ebp, ecx
	mov	ecx, [eax + 36]
	imul	ecx, [edi - 40]
	add	ebp, ecx
	mov	ecx, [eax + 32]
	imul	ecx, [edi - 36]
	add	ebp, ecx
	mov	ecx, [eax + 28]
	imul	ecx, [edi - 32]
	add	ebp, ecx
	mov	ecx, [eax + 24]
	imul	ecx, [edi - 28]
	add	ebp, ecx
	mov	ecx, [eax + 20]
	imul	ecx, [edi - 24]
	add	ebp, ecx
	mov	ecx, [eax + 16]
	imul	ecx, [edi - 20]
	add	ebp, ecx
	mov	ecx, [eax + 12]
	imul	ecx, [edi - 16]
	add	ebp, ecx
	mov	ecx, [eax + 8]
	imul	ecx, [edi - 12]
	add	ebp, ecx
	mov	ecx, [eax + 4]
	imul	ecx, [edi - 8]
	add	ebp, ecx
	mov	ecx, [eax]		;there is one byte missing
	imul	ecx, [edi - 4]
	add	ebp, ecx
.jumper_0:

	mov	cl, [esp + 36]
	sar	ebp, cl
	add	ebp, [esi + edi]
	mov	[edi], ebp
	add	edi, byte 4

	dec	ebx
	jz	short .end
	xor	ebp, ebp
	jmp	edx

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

; WATCHOUT: this routine works on 16 bit data which means bits-per-sample for
; the channel must be <= 16.  Especially note that this routine cannot be used
; for side-channel coded 16bps channels since the effective bps is 17.
	ALIGN	16
cident FLAC__lpc_restore_signal_asm_i386_mmx:
	;[esp + 40]	data[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	residual[]

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	esi, [esp + 20]
	mov	edi, [esp + 40]
	mov	eax, [esp + 32]
	mov	ebx, [esp + 24]

	test	ebx, ebx
	jz	.end				; do nothing if data_len == 0

	mov	edx, [esp + 28]
	movd	mm6, [esp + 36]
	mov	ebp, esp

	and	esp, 0xfffffff8

	xor	ecx, ecx
.copy_qlp_loop:
	push	word [edx + 4 * ecx]
	inc	ecx
	cmp	ecx, eax
	jnz	short .copy_qlp_loop

	and	ecx, 0x3
	test	ecx, ecx
	je	short .za_end
	sub	ecx, byte 4
.za_loop:
	push	word 0
	inc	eax
	inc	ecx
	jnz	short .za_loop
.za_end:

	movq	mm5, [esp + 2 * eax - 8]
	movd	mm4, [edi - 16]
	punpckldq	mm4, [edi - 12]
	movd	mm0, [edi - 8]
	punpckldq	mm0, [edi - 4]
	packssdw	mm4, mm0

	cmp	eax, byte 4
	jnbe	short .mmx_4more

	align	16
.mmx_4_loop_i:
	movq	mm7, mm4
	pmaddwd	mm7, mm5
	movq	mm0, mm7
	punpckhdq	mm7, mm7
	paddd	mm7, mm0
	psrad	mm7, mm6
	movd	mm1, [esi]
	paddd	mm7, mm1
	movd	[edi], mm7
	psllq	mm7, 48
	psrlq	mm4, 16
	por	mm4, mm7

	add	esi, byte 4
	add	edi, byte 4

	dec	ebx
	jnz	.mmx_4_loop_i
	jmp	.mmx_end
.mmx_4more:
	shl	eax, 2
	neg	eax
	add	eax, byte 16
	align	16
.mmx_4more_loop_i:
	mov	ecx, edi
	add	ecx, eax
	mov	edx, esp

	movq	mm7, mm4
	pmaddwd	mm7, mm5

	align	16
.mmx_4more_loop_j:
	movd	mm0, [ecx - 16]
	punpckldq	mm0, [ecx - 12]
	movd	mm1, [ecx - 8]
	punpckldq	mm1, [ecx - 4]
	packssdw	mm0, mm1
	pmaddwd	mm0, [edx]
	paddd	mm7, mm0

	add	edx, byte 8
	add	ecx, byte 16
	cmp	ecx, edi
	jnz	.mmx_4more_loop_j

	movq	mm0, mm7
	punpckhdq	mm7, mm7
	paddd	mm7, mm0
	psrad	mm7, mm6
	movd	mm1, [esi]
	paddd	mm7, mm1
	movd	[edi], mm7
	psllq	mm7, 48
	psrlq	mm4, 16
	por	mm4, mm7

	add	esi, byte 4
	add	edi, byte 4

	dec	ebx
	jnz	short .mmx_4more_loop_i
.mmx_end:
	emms
	mov	esp, ebp

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

end
