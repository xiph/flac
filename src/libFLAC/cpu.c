/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include "FLAC/config.h"
#include "private/cpu.h"
#include<stdlib.h>
#include<stdio.h>

const unsigned FLAC__CPUINFO_IA32_CPUID_CMOV = 0x00008000;
const unsigned FLAC__CPUINFO_IA32_CPUID_MMX = 0x00800000;
const unsigned FLAC__CPUINFO_IA32_CPUID_FXSR = 0x01000000;
const unsigned FLAC__CPUINFO_IA32_CPUID_SSE = 0x02000000;
const unsigned FLAC__CPUINFO_IA32_CPUID_SSE2 = 0x04000000;


void FLAC__cpu_info(FLAC__CPUInfo *info)
{
#ifdef FLAC__CPU_IA32
fprintf(stderr,"@@@ FLAC__CPU_IA32 defined\n");
	info->type = FLAC__CPUINFO_TYPE_IA32;
#if !defined FLAC__NO_ASM && defined FLAC__HAS_NASM
fprintf(stderr,"@@@ !defined FLAC__NO_ASM && defined FLAC__HAS_NASM\n");
	info->use_asm = true;
	{
		unsigned cpuid = FLAC__cpu_info_asm_i386();
		info->data.ia32.cmov = (cpuid & FLAC__CPUINFO_IA32_CPUID_CMOV)? true : false;
		info->data.ia32.mmx = (cpuid & FLAC__CPUINFO_IA32_CPUID_MMX)? true : false;
		info->data.ia32.fxsr = (cpuid & FLAC__CPUINFO_IA32_CPUID_FXSR)? true : false;
		info->data.ia32.sse = (cpuid & FLAC__CPUINFO_IA32_CPUID_SSE)? true : false;
		info->data.ia32.sse2 = (cpuid & FLAC__CPUINFO_IA32_CPUID_SSE2)? true : false;
fprintf(stderr,"@@@ \tcpuid=%08X\n",cpuid);
fprintf(stderr,"@@@ \tcmov=%u\n",info->data.ia32.cmov);
fprintf(stderr,"@@@ \tmmx=%u\n",info->data.ia32.mmx);
fprintf(stderr,"@@@ \tfxsr=%u\n",info->data.ia32.fxsr);
fprintf(stderr,"@@@ \tsse=%u\n",info->data.ia32.sse);
fprintf(stderr,"@@@ \tsse2=%u\n",info->data.ia32.sse2);
	}
#else
fprintf(stderr,"@@@ not: !defined FLAC__NO_ASM && defined FLAC__HAS_NASM\n");
	info->use_asm = true;
	info->use_asm = false;
#endif
#else
fprintf(stderr,"@@@ FLAC__CPU_UNKNOWN\n");
	info->type = FLAC__CPUINFO_TYPE_UNKNOWN;
	info->use_asm = false;
#endif
}
