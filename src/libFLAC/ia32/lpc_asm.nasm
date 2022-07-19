;  vim:filetype=nasm ts=8

;  libFLAC - Free Lossless Audio Codec library
;  Copyright (C) 2001-2009  Josh Coalson
;  Copyright (C) 2011-2022  Xiph.Org Foundation
;
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
;
;  - Redistributions of source code must retain the above copyright
;  notice, this list of conditions and the following disclaimer.
;
;  - Redistributions in binary form must reproduce the above copyright
;  notice, this list of conditions and the following disclaimer in the
;  documentation and/or other materials provided with the distribution.
;
;  - Neither the name of the Xiph.org Foundation nor the names of its
;  contributors may be used to endorse or promote products derived from
;  this software without specific prior written permission.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
;  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
;  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
;  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
;  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
;  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
;  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

%include "nasm.h"

	data_section

cglobal FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32
cglobal FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32_mmx
cglobal FLAC__lpc_compute_residual_from_qlp_coefficients_wide_asm_ia32

	code_section

;void FLAC__lpc_compute_residual_from_qlp_coefficients(const FLAC__int32 *data, unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 residual[])
;
;	for(i = 0; i < data_len; i++) {
;		sum = 0;
;		for(j = 0; j < order; j++)
;			sum += qlp_coeff[j] * data[i-j-1];
;		residual[i] = data[i] - (sum >> lp_quantization);
;	}
;
	ALIGN	16
cident FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32
	;[esp + 40]	residual[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	data[]

	;ASSERT(order > 0)

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
.begin:
	cmp	eax, byte 1
	jg	short .i_1more

	mov	ecx, [esp + 28]
	mov	edx, [ecx]			; edx = qlp_coeff[0]
	mov	eax, [esi - 4]			; eax = data[-1]
	mov	ecx, [esp + 36]			; cl = lp_quantization
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

	mov	ecx, [esp + 36]
	sar	ebp, cl
	neg	ebp
	add	ebp, [esi]
	mov	[edi], ebp
	add	esi, byte 4
	add	edi, byte 4

	dec	ebx
	jnz	.i_32more_loop_i

	jmp	.end

.mov_eip_to_eax:
	mov	eax, [esp]
	ret

.i_32:
	sub	edi, esi
	neg	eax
	lea	edx, [eax + eax * 8 + .jumper_0 - .get_eip0]
	call	.mov_eip_to_eax
.get_eip0:
	add	edx, eax
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
	mov	ecx, [eax]			; there is one byte missing
	imul	ecx, [esi - 4]
	add	ebp, ecx
.jumper_0:

	mov	ecx, [esp + 36]
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
; the channel and qlp_coeffs must be <= 16.  Especially note that this routine
; cannot be used for side-channel coded 16bps channels since the effective bps
; is 17.
	ALIGN	16
cident FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32_mmx
	;[esp + 40]	residual[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	data[]

	;ASSERT(order > 0)

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

	ALIGN	16
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

	ALIGN	16
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

	ALIGN	16
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
	jnz	near FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32.begin

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

; **********************************************************************
;
;void FLAC__lpc_compute_residual_from_qlp_coefficients_wide(const FLAC__int32 *data, unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 residual[])
; {
; 	unsigned i, j;
; 	FLAC__int64 sum;
;
; 	FLAC__ASSERT(order > 0);
;
;	for(i = 0; i < data_len; i++) {
;		sum = 0;
;		for(j = 0; j < order; j++)
;			sum += qlp_coeff[j] * (FLAC__int64)data[i-j-1];
;		residual[i] = data[i] - (FLAC__int32)(sum >> lp_quantization);
;	}
; }
	ALIGN	16
cident FLAC__lpc_compute_residual_from_qlp_coefficients_wide_asm_ia32
	;[esp + 40]	residual[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	data[]

	;ASSERT(order > 0)
	;ASSERT(order <= 32)
	;ASSERT(lp_quantization <= 31)

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	ebx, [esp + 24]			; ebx = data_len
	test	ebx, ebx
	jz	near .end			; do nothing if data_len == 0

.begin:
	mov	eax, [esp + 32]			; eax = order
	cmp	eax, 1
	jg	short .i_32

	mov	esi, [esp + 40]			; esi = residual[]
	mov	edi, [esp + 20]			; edi = data[]
	mov	ecx, [esp + 28]			; ecx = qlp_coeff[]
	mov	ebp, [ecx]			; ebp = qlp_coeff[0]
	mov	eax, [edi - 4]			; eax = data[-1]
	mov	ecx, [esp + 36]			; cl = lp_quantization
	ALIGN	16
.i_1_loop_i:
	imul	ebp				; edx:eax = qlp_coeff[0] * (FLAC__int64)data[i-1]
	shrd	eax, edx, cl			; 0 <= lp_quantization <= 15
	neg	eax
	add	eax, [edi]
	mov	[esi], eax
	mov	eax, [edi]
	add	esi, 4
	add	edi, 4
	dec	ebx
	jnz	.i_1_loop_i
	jmp	.end

.mov_eip_to_eax:
	mov	eax, [esp]
	ret

.i_32:	; eax = order
	neg	eax
	add	eax, eax
	lea	ebp, [eax + eax * 4 + .jumper_0 - .get_eip0]
	call	.mov_eip_to_eax
.get_eip0:
	add	ebp, eax
	inc	ebp				; compensate for the shorter opcode on the last iteration

	mov	ebx, [esp + 28]			; ebx = qlp_coeff[]
	mov	edi, [esp + 20]			; edi = data[]
	sub	[esp + 40], edi			; residual[] -= data[]

	xor	ecx, ecx
	xor	esi, esi
	jmp	ebp

;eax = --
;edx = --
;ecx = 0
;esi = 0
;
;ebx = qlp_coeff[]
;edi = data[]
;ebp = @address

	mov	eax, [ebx + 124]		; eax =  qlp_coeff[31]
	imul	dword [edi - 128]		; edx:eax =  qlp_coeff[31] * data[i-32]
	add	ecx, eax
	adc	esi, edx			; sum += qlp_coeff[31] * data[i-32]

	mov	eax, [ebx + 120]		; eax =  qlp_coeff[30]
	imul	dword [edi - 124]		; edx:eax =  qlp_coeff[30] * data[i-31]
	add	ecx, eax
	adc	esi, edx			; sum += qlp_coeff[30] * data[i-31]

	mov	eax, [ebx + 116]
	imul	dword [edi - 120]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 112]
	imul	dword [edi - 116]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 108]
	imul	dword [edi - 112]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 104]
	imul	dword [edi - 108]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 100]
	imul	dword [edi - 104]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 96]
	imul	dword [edi - 100]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 92]
	imul	dword [edi - 96]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 88]
	imul	dword [edi - 92]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 84]
	imul	dword [edi - 88]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 80]
	imul	dword [edi - 84]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 76]
	imul	dword [edi - 80]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 72]
	imul	dword [edi - 76]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 68]
	imul	dword [edi - 72]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 64]
	imul	dword [edi - 68]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 60]
	imul	dword [edi - 64]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 56]
	imul	dword [edi - 60]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 52]
	imul	dword [edi - 56]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 48]
	imul	dword [edi - 52]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 44]
	imul	dword [edi - 48]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 40]
	imul	dword [edi - 44]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 36]
	imul	dword [edi - 40]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 32]
	imul	dword [edi - 36]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 28]
	imul	dword [edi - 32]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 24]
	imul	dword [edi - 28]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 20]
	imul	dword [edi - 24]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 16]
	imul	dword [edi - 20]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 12]
	imul	dword [edi - 16]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 8]
	imul	dword [edi - 12]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx + 4]
	imul	dword [edi - 8]
	add	ecx, eax
	adc	esi, edx

	mov	eax, [ebx]			; eax =  qlp_coeff[ 0] (NOTE: one byte missing from instruction)
	imul	dword [edi - 4]			; edx:eax =  qlp_coeff[ 0] * data[i- 1]
	add	ecx, eax
	adc	esi, edx			; sum += qlp_coeff[ 0] * data[i- 1]

.jumper_0:
	mov	edx, ecx
;esi:edx = sum
	mov	ecx, [esp + 36]			; cl = lp_quantization
	shrd	edx, esi, cl			; edx = (sum >> lp_quantization)
;eax = --
;ecx = --
;edx = sum >> lp_q
;esi = --
	neg	edx				; edx = -(sum >> lp_quantization)
	mov	eax, [esp + 40]			; residual[] - data[]
	add	edx, [edi]			; edx = data[i] - (sum >> lp_quantization)
	mov	[edi + eax], edx
	add	edi, 4

	dec	dword [esp + 24]
	jz	short .end
	xor	ecx, ecx
	xor	esi, esi
	jmp	ebp

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

; end
