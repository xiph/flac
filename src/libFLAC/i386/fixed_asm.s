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

cglobal FLAC__fixed_compute_best_predictor_asm_i386_mmx_cmov

	code_section

; **********************************************************************
;
; unsigned FLAC__fixed_compute_best_predictor(const int32 data[], unsigned data_len, real residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
; {
; 	int32 last_error_0 = data[-1];
; 	int32 last_error_1 = data[-1] - data[-2];
; 	int32 last_error_2 = last_error_1 - (data[-2] - data[-3]);
; 	int32 last_error_3 = last_error_2 - (data[-2] - 2*data[-3] + data[-4]);
; 	int32 error, save;
; 	uint32 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0;
; 	unsigned i, order;
;
; 	for(i = 0; i < data_len; i++) {
; 		error  = data[i]     ; total_error_0 += local_abs(error);                      save = error;
; 		error -= last_error_0; total_error_1 += local_abs(error); last_error_0 = save; save = error;
; 		error -= last_error_1; total_error_2 += local_abs(error); last_error_1 = save; save = error;
; 		error -= last_error_2; total_error_3 += local_abs(error); last_error_2 = save; save = error;
; 		error -= last_error_3; total_error_4 += local_abs(error); last_error_3 = save;
; 	}
;
; 	if(total_error_0 < min(min(min(total_error_1, total_error_2), total_error_3), total_error_4))
; 		order = 0;
; 	else if(total_error_1 < min(min(total_error_2, total_error_3), total_error_4))
; 		order = 1;
; 	else if(total_error_2 < min(total_error_3, total_error_4))
; 		order = 2;
; 	else if(total_error_3 < total_error_4)
; 		order = 3;
; 	else
; 		order = 4;
;
; 	residual_bits_per_sample[0] = (real)((data_len > 0 && total_error_0 > 0) ? log(M_LN2 * (real)total_error_0  / (real) data_len) / M_LN2 : 0.0);
; 	residual_bits_per_sample[1] = (real)((data_len > 0 && total_error_1 > 0) ? log(M_LN2 * (real)total_error_1  / (real) data_len) / M_LN2 : 0.0);
; 	residual_bits_per_sample[2] = (real)((data_len > 0 && total_error_2 > 0) ? log(M_LN2 * (real)total_error_2  / (real) data_len) / M_LN2 : 0.0);
; 	residual_bits_per_sample[3] = (real)((data_len > 0 && total_error_3 > 0) ? log(M_LN2 * (real)total_error_3  / (real) data_len) / M_LN2 : 0.0);
; 	residual_bits_per_sample[4] = (real)((data_len > 0 && total_error_4 > 0) ? log(M_LN2 * (real)total_error_4  / (real) data_len) / M_LN2 : 0.0);
;
; 	return order;
; }
	ALIGN 16
cident FLAC__fixed_compute_best_predictor_asm_i386_mmx_cmov

	; esp + 36 == data[]
	; esp + 40 == data_len
	; esp + 44 == residual_bits_per_sample[]

	push	ebp
	push	ebx
	push	esi
	push	edi
	sub	esp, byte 16
	; qword [esp] == temp space for loading uint64s to FPU regs
	; dword [esp] == last_error_0
	; dword [esp + 4] == last_error_1
	; dword [esp + 8] == last_error_2
	; dword [esp + 12] == last_error_3

	; eax == error
	; ebx == &data[i]
	; ecx == loop counter (i)
	; edx == temp
	; edi == save
	; ebp == order
	; mm0 == total_error_1:total_error_0
	; mm1 == total_error_3:total_error_2
	; mm2 == 0:total_error_4
	; mm3/4 == 0:unpackarea
	; mm5 == abs(error_1):abs(error_0)
	; mm5 == abs(error_3):abs(error_2)

	pxor	mm0, mm0			; total_error_1 = total_error_0 = 0
	pxor	mm1, mm1			; total_error_3 = total_error_2 = 0
	pxor	mm2, mm2			; total_error_4 = 0
	mov	ebx, [esp + 36]			; ebx = data[]
	mov	ecx, [ebx - 4]			; ecx == data[-1]  last_error_0 = data[-1]
	mov	eax, [ebx - 8]			; eax == data[-2]
	mov	ebp, [ebx - 16]			; ebp == data[-4]
	mov	ebx, [ebx - 12]			; ebx == data[-3]
	mov	edx, ecx
	sub	edx, eax			; last_error_1 = data[-1] - data[-2]
	mov	esi, edx
	sub	esi, eax
	add	esi, ebx			; last_error_2 = last_error_1 - (data[-2] - data[-3])
	shl	ebx, 1
	mov	edi, esi
	sub	edi, eax
	add	edi, ebx
	sub	edi, ebp			; last_error_3 = last_error_2 - (data[-2] - 2*data[-3] + data[-4]);
	mov	ebx, [esp + 36]			; ebx = data[]
	mov	[esp], ecx			; [esp] = last_error_0
	mov	[esp + 4], edx			; [esp + 4] = last_error_1
	mov	[esp + 8], esi			; [esp + 8] = last_error_2
	mov	[esp + 12], edi			; [esp + 12] = last_error_3
	mov	ecx, [esp + 40]			; ecx = data_len

	; 	for(i = 0; i < data_len; i++) {
	; 		error_0  = data[i]     ;                      save = error_0; total_error_0 += local_abs(error_0);
	; 		error_1 -= last_error_0; last_error_0 = save; save = error_1; total_error_1 += local_abs(error_1);
	; 		error_2 -= last_error_1; last_error_1 = save; save = error_2; total_error_2 += local_abs(error_2);
	; 		error_3 -= last_error_2; last_error_2 = save; save = error_3; total_error_3 += local_abs(error_3);
	; 		error_4 -= last_error_3; last_error_3 = save;                 total_error_4 += local_abs(error_4);
	; 	}
	ALIGN 16
.loop:
	mov	eax, [ebx]			; eax = error_0 = data[i]
	add	ebx, 4
	mov	edi, eax			; edi == save = error_0
	mov	edx, eax			; edx = error_0
	neg	edx				; edx = -error_0
	cmovns	eax, edx			; eax = abs(error_0)
	movd	mm5, eax			; mm5 = 0:abs(error_0)
	mov	edx, [esp]			; edx = last_error_0
	mov	eax, edi			; eax = error(error_0)
	mov	[esp], edi			; [esp] == last_error_0 = save
	sub	eax, edx			; error -= last_error_0
	mov	edi, eax			; edi == save = error_1
	mov	edx, eax			; edx = error_1
	neg	edx				; edx = -error_1
	cmovns	eax, edx			; eax = abs(error_1)
	movd	mm4, eax			; mm4 = 0:abs(error_1)
	punpckldq	mm5, mm4		; mm5 = abs(error_1):abs(error_0)
	mov	edx, [esp + 4]			; edx = last_error_1
	mov	eax, edi			; eax = error(error_1)
	mov	[esp + 4], edi			; [esp + 4] == last_error_1 = save
	sub	eax, edx			; error -= last_error_1
	mov	edi, eax			; edi == save = error_2
	mov	edx, eax			; edx = error_2
	paddd	mm0, mm5			; [CR] total_error_1 += abs(error_1) ; total_error_0 += abs(error_0)
	neg	edx				; edx = -error_2
	cmovns	eax, edx			; eax = abs(error_2)
	movd	mm5, eax			; mm5 = 0:abs(error_2)
	mov	edx, [esp + 8]			; edx = last_error_2
	mov	eax, edi			; eax = error(error_2)
	mov	[esp + 8], edi			; [esp + 8] == last_error_2 = save
	sub	eax, edx			; error -= last_error_2
	mov	edi, eax			; edi == save = error_3
	mov	edx, eax			; edx = error_3
	neg	edx				; edx = -error_3
	cmovns	eax, edx			; eax = abs(error_3)
	movd	mm4, eax			; mm4 = 0:abs(error_3)
	punpckldq	mm5, mm4		; mm5 = abs(error_3):abs(error_2)
	mov	edx, [esp + 12]			; edx = last_error_3
	mov	eax, edi			; eax = error(error_3)
	mov	[esp + 12], edi			; [esp + 12] == last_error_3 = save
	sub	eax, edx			; error -= last_error_3
	mov	edx, eax			; edx = error_4
	paddd	mm1, mm5			; [CR] total_error_3 += abs(error_3) ; total_error_2 += abs(error_2)
	neg	edx				; edx = -error_4
	cmovns	eax, edx			; eax = abs(error_4)
	movd	mm5, eax			; mm5 = 0:abs(error_4)
	paddd	mm2, mm5			; total_error_4 += abs(error_4)
	dec	ecx
	jnz	near .loop

; 	if(total_error_0 < min(min(min(total_error_1, total_error_2), total_error_3), total_error_4))
; 		order = 0;
; 	else if(total_error_1 < min(min(total_error_2, total_error_3), total_error_4))
; 		order = 1;
; 	else if(total_error_2 < min(total_error_3, total_error_4))
; 		order = 2;
; 	else if(total_error_3 < total_error_4)
; 		order = 3;
; 	else
; 		order = 4;
	movd	edi, mm2			; edi = total_error_4
	movq	mm4, mm1			; mm4 = total_error_3:total_error_2
	psrlq	mm4, 32				; mm4 = 0:total_error_3
	movd	edx, mm1			; edx = total_error_2
	movd	esi, mm4			; esi = total_error_3
	movq	mm3, mm0			; mm3 = total_error_1:total_error_0
	psrlq	mm3, 32				; mm3 = 0:total_error_1
	movd	ebx, mm0			; ebx = total_error_0
	movd	ecx, mm3			; ecx = total_error_1
	emms
	mov	eax, ebx			; eax = total_error_0
	cmp	ecx, ebx
	cmovb	eax, ecx			; eax = min(total_error_0, total_error_1)
	cmp	edx, eax
	cmovb	eax, edx			; eax = min(total_error_0, total_error_1, total_error_2)
	cmp	esi, eax
	cmovb	eax, esi			; eax = min(total_error_0, total_error_1, total_error_2, total_error_3)
	cmp	edi, eax
	cmovb	eax, edi			; eax = min(total_error_0, total_error_1, total_error_2, total_error_3, total_error_4)

	cmp	eax, ebx
	jne	.not_order_0
	xor	ebp, ebp
	jmp	short .got_order
.not_order_0:
	cmp	eax, ecx
	jne	.not_order_1
	mov	ebp, 1
	jmp	short .got_order
.not_order_1:
	cmp	eax, edx
	jne	.not_order_2
	mov	ebp, 2
	jmp	short .got_order
.not_order_2:
	cmp	eax, esi
	jne	.not_order_3
	mov	ebp, 3
	jmp	short .got_order
.not_order_3:
	mov	ebp, 4
.got_order:
	; 	residual_bits_per_sample[0] = (real)((data_len > 0 && total_error_0 > 0) ? log(M_LN2 * (real)total_error_0  / (real) data_len) / M_LN2 : 0.0);
	; 	residual_bits_per_sample[1] = (real)((data_len > 0 && total_error_1 > 0) ? log(M_LN2 * (real)total_error_1  / (real) data_len) / M_LN2 : 0.0);
	; 	residual_bits_per_sample[2] = (real)((data_len > 0 && total_error_2 > 0) ? log(M_LN2 * (real)total_error_2  / (real) data_len) / M_LN2 : 0.0);
	; 	residual_bits_per_sample[3] = (real)((data_len > 0 && total_error_3 > 0) ? log(M_LN2 * (real)total_error_3  / (real) data_len) / M_LN2 : 0.0);
	; 	residual_bits_per_sample[4] = (real)((data_len > 0 && total_error_4 > 0) ? log(M_LN2 * (real)total_error_4  / (real) data_len) / M_LN2 : 0.0);
	xor	eax, eax
	cmp	eax, [esp + 40]
	je	near .data_len_is_0
	fild	dword [esp + 40]		; ST = data_len (NOTE: assumes data_len is <2gigs)
.rbps_0:
	test	ebx, ebx
	jz	.total_error_0_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], ebx
	mov	[esp + 4], eax			; [esp] = (uint64)total_error_0
	mov	ebx, [esp + 44]
	fild	qword [esp]			; ST = total_error_0 1.0 data_len
	fdiv	st2				; ST = total_error_0/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_0/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_0/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_0/data_len) data_len
	fstp	dword [ebx]			; residual_bits_per_sample[0] = log2(ln2*total_error_0/data_len)   ST = data_len
	jmp	short .rbps_1
.total_error_0_is_0:
	mov	ebx, [esp + 44]
	mov	[ebx], eax			; residual_bits_per_sample[0] = 0.0
.rbps_1:
	test	ecx, ecx
	jz	.total_error_1_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], ecx
	mov	[esp + 4], eax			; [esp] = (uint64)total_error_1
	fild	qword [esp]			; ST = total_error_1 1.0 data_len
	fdiv	st2				; ST = total_error_1/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_1/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_1/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_1/data_len) data_len
	fstp	dword [ebx + 4]			; residual_bits_per_sample[1] = log2(ln2*total_error_1/data_len)   ST = data_len
	jmp	short .rbps_2
.total_error_1_is_0:
	mov	[ebx + 4], eax			; residual_bits_per_sample[1] = 0.0
.rbps_2:
	test	edx, edx
	jz	.total_error_2_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], edx
	mov	[esp + 4], eax			; [esp] = (uint64)total_error_2
	fild	qword [esp]			; ST = total_error_2 1.0 data_len
	fdiv	st2				; ST = total_error_2/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_2/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_2/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_2/data_len) data_len
	fstp	dword [ebx + 8]			; residual_bits_per_sample[2] = log2(ln2*total_error_2/data_len)   ST = data_len
	jmp	short .rbps_3
.total_error_2_is_0:
	mov	[ebx + 8], eax			; residual_bits_per_sample[2] = 0.0
.rbps_3:
	test	esi, esi
	jz	.total_error_3_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], esi
	mov	[esp + 4], eax			; [esp] = (uint64)total_error_3
	fild	qword [esp]			; ST = total_error_3 1.0 data_len
	fdiv	st2				; ST = total_error_3/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_3/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_3/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_3/data_len) data_len
	fstp	dword [ebx + 12]		; residual_bits_per_sample[3] = log2(ln2*total_error_3/data_len)   ST = data_len
	jmp	short .rbps_4
.total_error_3_is_0:
	mov	[ebx + 12], eax			; residual_bits_per_sample[3] = 0.0
.rbps_4:
	test	edi, edi
	jz	.total_error_4_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], edi
	mov	[esp + 4], eax			; [esp] = (uint64)total_error_4
	fild	qword [esp]			; ST = total_error_4 1.0 data_len
	fdiv	st2				; ST = total_error_4/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_4/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_4/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_4/data_len) data_len
	fstp	dword [ebx + 16]		; residual_bits_per_sample[4] = log2(ln2*total_error_4/data_len)   ST = data_len
	jmp	short .rbps_end
.total_error_4_is_0:
	mov	[ebx + 16], eax			; residual_bits_per_sample[4] = 0.0
.rbps_end:
	fstp	st0				; ST = [empty]
	jmp	short .end
.data_len_is_0:
	; data_len == 0, so residual_bits_per_sample[*] = 0.0
	mov	ecx, 5				; eax still == 0, ecx = # of dwords of 0 to store
	mov	edi, [esp + 44]
	rep stosd

.end:
	mov	eax, ebp			; return order
	add	esp, byte 16
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

end
