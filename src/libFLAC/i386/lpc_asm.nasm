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

cglobal FLAC__lpc_compute_autocorrelation_asm
cglobal FLAC__lpc_restore_signal_asm

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
FLAC__lpc_compute_autocorrelation_asm:

	; esp + 20 == data[]
	; esp + 24 == data_len
	; esp + 28 == lag
	; esp + 32 == autoc[]

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
	jnz	.outer_loop

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

; NOTE: this SSE version is not even tested yet and only works for lag == 8
FLAC__lpc_compute_autocorrelation_sse:

	; esp + 4 == data[]
	; esp + 8 == data_len
	; esp + 12 == lag
	; esp + 16 == autoc[]

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
	movps	xmm4, xmm0
	movps	xmm5, xmm1			; xmm5:xmm4 = xmm1:xmm0 = data[sample]*8
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
	shufps	xmm2, xmm4, 2103h		; xmm2
	shufps	xmm3, xmm5, 2103h		; xmm2
	movss	xmm3, xmm2
	movss	xmm2, xmm0

	movps	xmm4, xmm0
	movps	xmm5, xmm1			; xmm5:xmm4 = xmm1:xmm0 = data[sample]*8
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
FLAC__lpc_restore_signal_asm:
	; [esp + 20] == residual[]
	; [esp + 24] == data_len
	; [esp + 28] == qlp_coeff[]
	; [esp + 32] == order
	; [esp + 36] == lp_quantization
	; [esp + 40] == data[]

	push	ebp
	push	ebx
	push	esi
	push	edi
	
	mov	esi, [esp + 20]
	mov	ebx, [esp + 24]
	mov	eax, [esp + 32]
	mov	edi, [esp + 40]
	
	cmp	eax, byte 32		; for order <= 32 there is a faster routine
	jbe	short .b

	; This version is here just for completeness, since FLAC__MAX_LPC_ORDER == 32
	ALIGN 16
.loop_i_a
	xor	ebp, ebp
	mov	ecx, [esp + 32]
	mov	edx, ecx
	shl	edx, 2
	add	edx, [esp + 28]
	neg	ecx
	ALIGN 16
.loop_j_a
	sub	edx, 4
	mov	eax, [edx]
	imul	eax, [edi + 4 * ecx]
	add	ebp, eax
	inc	ecx
	jnz	.loop_j_a
	
	mov	cl, [esp + 36]
	sar	ebp, cl
	add	ebp, [esi]
	mov	[edi], ebp
	add	edi, byte 4
	add	esi, byte 4
	
	dec	ebx
	jnz	.loop_i_a
	
	jmp	.end

.b
	sub	esi, edi
	neg	eax
	lea	edx, [eax + eax * 8 + .jumper_0]
	inc	edx
	mov	eax, [esp + 28]
	xor	ebp, ebp
	jmp	edx
;.jumper_32
	mov	ecx, [eax + 124]	;32
	imul	ecx, [edi - 128]
	add	ebp, ecx
	mov	ecx, [eax + 120]
	imul	ecx, [edi - 124]
	add	ebp, ecx
	mov	ecx, [eax + 116]	;30
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
	mov	ecx, [eax + 96]		;25
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
	mov	ecx, [eax + 76]		;20
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
	mov	ecx, [eax + 56]		;15
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
	mov	ecx, [eax + 36]		;10
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
	mov	ecx, [eax + 16]		;5
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
.jumper_0

	mov	cl, [esp + 36]
	sar	ebp, cl
	add	ebp, [esi + edi]
	mov	[edi], ebp
	add	edi, byte 4
	
	dec	ebx
	jz	short .end
	xor	ebp, ebp
	jmp	edx
	
.end
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

end
