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

	code_section

; **********************************************************************
;
; void FLAC__lpc_compute_autocorrelation_asm(const real data[], unsigned data_len, unsigned lag, real autoc[])
; {
; 	real d;
; 	unsigned i;
;
; 	while(lag--) {
; 		for(i = lag, d = 0.0; i < data_len; i++)
; 			d += data[i] * data[i - lag];
; 		autoc[lag] = d;
; 	}
; }
;
FLAC__lpc_compute_autocorrelation_asm:

	push	ebp
	lea	ebp, [esp + 8]
	push	eax
	push	ebx
	push	ecx
	push	edx
	push	esi
	push	edi

	mov	esi, [ebp]			; esi == data
	mov	ecx, [ebp + 4]			; ecx == data_len
	mov	edx, [ebp + 8]			; edx == lag
	mov	edi, [ebp + 12]			; edi == autoc

.outer_loop:
	test	edx, edx
	jz	.outer_end
	dec	edx				; lag--

	mov	ebx, edx			; ebx == i <- lag
	xor	eax, eax			; eax == i-lag
	fldz					; ST = d <- 0.0
.inner_loop:
	cmp	ebx, ecx
	jae	short .inner_end
	fld	qword [esi + ebx * 8]		; ST = data[i] d
	fmul	qword [esi + eax * 8]		; ST = data[i]*data[i-lag] d
	faddp	st1				; d += data[i]*data[i-lag]  ST = d
	inc	eax
	inc	ebx
	jmp	.inner_loop
.inner_end:

	fstp	qword [edi + edx * 8]

	jmp	.outer_loop
.outer_end:

	pop	edi
	pop	esi
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	pop	ebp
	ret

end
