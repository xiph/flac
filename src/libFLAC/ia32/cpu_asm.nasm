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

cglobal FLAC__cpu_info_asm_ia32
cglobal FLAC__cpu_info_extended_amd_asm_ia32
cglobal FLAC__cpu_info_sse_os_asm_ia32

	code_section

; **********************************************************************
;

have_cpuid:
	pushfd
	pop	eax
	mov	edx, eax
	xor	eax, 0x00200000
	push	eax
	popfd
	pushfd
	pop	eax
	cmp	eax, edx
	jz	.no_cpuid
	mov	eax, 1
	jmp	.end
.no_cpuid:
	xor	eax, eax
.end:
	ret

cident FLAC__cpu_info_asm_ia32
	push	ebx
	call	have_cpuid
	test	eax, eax
	jz	.no_cpuid
	mov	eax, 1
	cpuid
	mov	eax, edx
	jmp	.end
.no_cpuid:
	xor	eax, eax
.end
	pop	ebx
	ret

cident FLAC__cpu_info_extended_amd_asm_ia32
	push	ebx
	call	have_cpuid
	test	eax, eax
	jz	.no_cpuid
	mov	eax, 0x80000000
	cpuid
	cmp	eax, 0x80000001
	jb	.no_cpuid
	mov	eax, 0x80000001
	cpuid
	mov	eax, edx
	jmp	.end
.no_cpuid
	xor	eax, eax
.end
	pop	ebx
	ret

;WATCHOUT - DO NOT call this function until you have verified CPU support of
;           SSE by inspecting the return value from FLAC__cpu_info_asm_ia32
;NOTE - Since we're not in priv level 0 we can't just check CR4 bits 9 & 10,
;       so right now we just assume there is no OS support.  If you know
;       how to write code to trap a #UD exception in nasm so we can implement
;       this function correctly, let us know!
cident FLAC__cpu_info_sse_os_asm_ia32
	push	ebx
	mov	eax, 1
	cpuid
	mov	eax, 0		;we would like to 'move eax, cr4'
	shr	eax, 9
	and	eax, 3
	pop	ebx
	ret

end
