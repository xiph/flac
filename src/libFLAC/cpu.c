/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001-2009  Josh Coalson
 * Copyright (C) 2011-2013  Xiph.Org Foundation
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "private/cpu.h"
#include <stdlib.h>
#include <stdio.h>

#if defined FLAC__CPU_IA32
# include <signal.h>

static void disable_sse(FLAC__CPUInfo *info)
{
	info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
}

#endif

#if defined (__NetBSD__) || defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(__APPLE__)
/* how to get sysctlbyname()? */
#endif

#ifdef FLAC__CPU_IA32
/* these are flags in EDX of CPUID AX=00000001 */
static const unsigned FLAC__CPUINFO_IA32_CPUID_CMOV = 0x00008000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_MMX = 0x00800000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_FXSR = 0x01000000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSE = 0x02000000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSE2 = 0x04000000;
#endif
/* these are flags in ECX of CPUID AX=00000001 */
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSE3 = 0x00000001;
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSSE3 = 0x00000200;
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSE41 = 0x00080000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_SSE42 = 0x00100000;
#ifdef FLAC__CPU_IA32
/* these are flags in EDX of CPUID AX=80000001 */
static const unsigned FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_3DNOW = 0x80000000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_EXT3DNOW = 0x40000000;
static const unsigned FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_EXTMMX = 0x00400000;
#endif

/*
 * Extra stuff needed for detection of OS support for SSE on IA-32
 */
#if defined(FLAC__CPU_IA32) && !defined FLAC__NO_ASM && (defined FLAC__HAS_NASM || defined FLAC__HAS_X86INTRIN) && !defined FLAC__NO_SSE_OS && !defined FLAC__SSE_OS
# if defined(__linux__)
/*
 * If the OS doesn't support SSE, we will get here with a SIGILL.  We
 * modify the return address to jump over the offending SSE instruction
 * and also the operation following it that indicates the instruction
 * executed successfully.  In this way we use no global variables and
 * stay thread-safe.
 *
 * 3 + 3 + 6:
 *   3 bytes for "xorps xmm0,xmm0"
 *   3 bytes for estimate of how long the follwing "inc var" instruction is
 *   6 bytes extra in case our estimate is wrong
 * 12 bytes puts us in the NOP "landing zone"
 */
#   include <sys/ucontext.h>
	static void sigill_handler_sse_os(int signal, siginfo_t *si, void *uc)
	{
		(void)signal, (void)si;
		((ucontext_t*)uc)->uc_mcontext.gregs[14/*REG_EIP*/] += 3 + 3 + 6;
	}
# elif defined(_MSC_VER)
#  include <windows.h>
# endif
#endif


void FLAC__cpu_info(FLAC__CPUInfo *info)
{
/*
 * IA32-specific
 */
#ifdef FLAC__CPU_IA32
	info->type = FLAC__CPUINFO_TYPE_IA32;
#if !defined FLAC__NO_ASM && (defined FLAC__HAS_NASM || defined FLAC__HAS_X86INTRIN)
	info->use_asm = true; /* we assume a minimum of 80386 with FLAC__CPU_IA32 */
#ifdef FLAC__HAS_NASM
	info->ia32.cpuid = FLAC__cpu_have_cpuid_asm_ia32()? true : false;
#else
	info->ia32.cpuid = FLAC__cpu_have_cpuid_x86()? true : false;
#endif
	info->ia32.bswap = info->ia32.cpuid; /* CPUID => BSWAP since it came after */
	info->ia32.cmov = false;
	info->ia32.mmx = false;
	info->ia32.fxsr = false;
	info->ia32.sse = false;
	info->ia32.sse2 = false;
	info->ia32.sse3 = false;
	info->ia32.ssse3 = false;
	info->ia32.sse41 = false;
	info->ia32.sse42 = false;
	info->ia32._3dnow = false;
	info->ia32.ext3dnow = false;
	info->ia32.extmmx = false;
	if(info->ia32.cpuid) {
		/* http://www.sandpile.org/x86/cpuid.htm */
		FLAC__uint32 flags_edx, flags_ecx;
#ifdef FLAC__HAS_NASM
		FLAC__cpu_info_asm_ia32(&flags_edx, &flags_ecx);
#else
		FLAC__cpu_info_x86(&flags_edx, &flags_ecx);
#endif
		info->ia32.cmov  = (flags_edx & FLAC__CPUINFO_IA32_CPUID_CMOV )? true : false;
		info->ia32.mmx   = (flags_edx & FLAC__CPUINFO_IA32_CPUID_MMX  )? true : false;
		info->ia32.fxsr  = (flags_edx & FLAC__CPUINFO_IA32_CPUID_FXSR )? true : false;
		info->ia32.sse   = (flags_edx & FLAC__CPUINFO_IA32_CPUID_SSE  )? true : false;
		info->ia32.sse2  = (flags_edx & FLAC__CPUINFO_IA32_CPUID_SSE2 )? true : false;
		info->ia32.sse3  = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSE3 )? true : false;
		info->ia32.ssse3 = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSSE3)? true : false;
		info->ia32.sse41 = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSE41)? true : false;
		info->ia32.sse42 = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSE42)? true : false;

#if defined FLAC__HAS_NASM && defined FLAC__USE_3DNOW
		flags_edx = FLAC__cpu_info_extended_amd_asm_ia32();
		info->ia32._3dnow   = (flags_edx & FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_3DNOW   )? true : false;
		info->ia32.ext3dnow = (flags_edx & FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_EXT3DNOW)? true : false;
		info->ia32.extmmx   = (flags_edx & FLAC__CPUINFO_IA32_CPUID_EXTENDED_AMD_EXTMMX  )? true : false;
#else
		info->ia32._3dnow = info->ia32.ext3dnow = info->ia32.extmmx = false;
#endif

#ifdef DEBUG
		fprintf(stderr, "CPU info (IA-32):\n");
		fprintf(stderr, "  CPUID ...... %c\n", info->ia32.cpuid   ? 'Y' : 'n');
		fprintf(stderr, "  BSWAP ...... %c\n", info->ia32.bswap   ? 'Y' : 'n');
		fprintf(stderr, "  CMOV ....... %c\n", info->ia32.cmov    ? 'Y' : 'n');
		fprintf(stderr, "  MMX ........ %c\n", info->ia32.mmx     ? 'Y' : 'n');
		fprintf(stderr, "  FXSR ....... %c\n", info->ia32.fxsr    ? 'Y' : 'n');
		fprintf(stderr, "  SSE ........ %c\n", info->ia32.sse     ? 'Y' : 'n');
		fprintf(stderr, "  SSE2 ....... %c\n", info->ia32.sse2    ? 'Y' : 'n');
		fprintf(stderr, "  SSE3 ....... %c\n", info->ia32.sse3    ? 'Y' : 'n');
		fprintf(stderr, "  SSSE3 ...... %c\n", info->ia32.ssse3   ? 'Y' : 'n');
		fprintf(stderr, "  SSE41 ...... %c\n", info->ia32.sse41   ? 'Y' : 'n');
		fprintf(stderr, "  SSE42 ...... %c\n", info->ia32.sse42   ? 'Y' : 'n');
		fprintf(stderr, "  3DNow! ..... %c\n", info->ia32._3dnow  ? 'Y' : 'n');
		fprintf(stderr, "  3DNow!-ext . %c\n", info->ia32.ext3dnow? 'Y' : 'n');
		fprintf(stderr, "  3DNow!-MMX . %c\n", info->ia32.extmmx  ? 'Y' : 'n');
#endif

		/*
		 * now have to check for OS support of SSE instructions
		 */
		if(info->ia32.sse) {
#if defined FLAC__NO_SSE_OS
			/* assume user knows better than us; turn it off */
			disable_sse(info);
#elif defined FLAC__SSE_OS
			/* assume user knows better than us; leave as detected above */
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__) || defined(__APPLE__)
			int sse = 0;
			size_t len;
			/* at least one of these must work: */
			len = sizeof(sse); sse = sse || (sysctlbyname("hw.instruction_sse", &sse, &len, NULL, 0) == 0 && sse);
			len = sizeof(sse); sse = sse || (sysctlbyname("hw.optional.sse"   , &sse, &len, NULL, 0) == 0 && sse); /* __APPLE__ ? */
			if(!sse)
				disable_sse(info);
#elif defined(__NetBSD__) || defined (__OpenBSD__)
# if __NetBSD_Version__ >= 105250000 || (defined __OpenBSD__)
			int val = 0, mib[2] = { CTL_MACHDEP, CPU_SSE };
			size_t len = sizeof(val);
			if(sysctl(mib, 2, &val, &len, NULL, 0) < 0 || !val)
				disable_sse(info);
			else { /* double-check SSE2 */
				mib[1] = CPU_SSE2;
				len = sizeof(val);
				if(sysctl(mib, 2, &val, &len, NULL, 0) < 0 || !val) {
					disable_sse(info);
					info->ia32.fxsr = info->ia32.sse = true;
				}
			}
# else
			disable_sse(info);
# endif
#elif defined(__linux__)
			int sse = 0;
			struct sigaction sigill_save;
			struct sigaction sigill_sse;
			sigill_sse.sa_sigaction = sigill_handler_sse_os;
			__sigemptyset(&sigill_sse.sa_mask);
			sigill_sse.sa_flags = SA_SIGINFO | SA_RESETHAND; /* SA_RESETHAND just in case our SIGILL return jump breaks, so we don't get stuck in a loop */
			if(0 == sigaction(SIGILL, &sigill_sse, &sigill_save))
			{
				/* http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html */
				/* see sigill_handler_sse_os() for an explanation of the following: */
				asm volatile (
					"xorps %%xmm0,%%xmm0\n\t" /* will cause SIGILL if unsupported by OS */
					"incl %0\n\t"             /* SIGILL handler will jump over this */
					/* landing zone */
					"nop\n\t" /* SIGILL jump lands here if "inc" is 9 bytes */
					"nop\n\t"
					"nop\n\t"
					"nop\n\t"
					"nop\n\t"
					"nop\n\t"
					"nop\n\t" /* SIGILL jump lands here if "inc" is 3 bytes (expected) */
					"nop\n\t"
					"nop"     /* SIGILL jump lands here if "inc" is 1 byte */
					: "=r"(sse)
					: "0"(sse)
				);

				sigaction(SIGILL, &sigill_save, NULL);
			}

			if(!sse)
				disable_sse(info);
#elif defined(_MSC_VER)
			__try {
				__asm {
					xorps xmm0,xmm0
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER) {
				if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
					disable_sse(info);
			}
#elif defined(__GNUC__) /* MinGW goes here */
			int sse = 0;
			/* Based on the idea described in Agner Fog's manual "Optimizing subroutines in assembly language" */
			/* In theory, not guaranteed to detect lack of OS SSE support on some future Intel CPUs, but in practice works (see the aforementioned manual) */
			if (info->ia32.fxsr) {
				struct {
					FLAC__uint32 buff[128];
				} __attribute__((aligned(16))) fxsr;
				FLAC__uint32 old_val, new_val;

				asm volatile ("fxsave %0"  : "=m" (fxsr) : "m" (fxsr));
				old_val = fxsr.buff[50];
				fxsr.buff[50] ^= 0x0013c0de;                             /* change value in the buffer */
				asm volatile ("fxrstor %0" : "=m" (fxsr) : "m" (fxsr));  /* try to change SSE register */
				fxsr.buff[50] = old_val;                                 /* restore old value in the buffer */
				asm volatile ("fxsave %0 " : "=m" (fxsr) : "m" (fxsr));  /* old value will be overwritten if SSE register was changed */
				new_val = fxsr.buff[50];                                 /* == old_val if FXRSTOR didn't change SSE register and (old_val ^ 0x0013c0de) otherwise */
				fxsr.buff[50] = old_val;                                 /* again restore old value in the buffer */
				asm volatile ("fxrstor %0" : "=m" (fxsr) : "m" (fxsr));  /* restore old values of registers */

				if ((old_val^new_val) == 0x0013c0de)
					sse = 1;
			}
			if(!sse)
				disable_sse(info);
#else
			/* no way to test, disable to be safe */
			disable_sse(info);
#endif
#ifdef DEBUG
			fprintf(stderr, "  SSE OS sup . %c\n", info->ia32.sse     ? 'Y' : 'n');
#endif
		}
		else /* info->ia32.sse == false */
			disable_sse(info);
	}
#else
	info->use_asm = false;
#endif

/*
 * x86-64-specific
 */
#elif defined FLAC__CPU_X86_64
	info->type = FLAC__CPUINFO_TYPE_X86_64;
#if !defined FLAC__NO_ASM && defined FLAC__HAS_X86INTRIN
	info->use_asm = true;
	{
		/* http://www.sandpile.org/x86/cpuid.htm */
		FLAC__uint32 flags_edx, flags_ecx;
		FLAC__cpu_info_x86(&flags_edx, &flags_ecx);
		info->x86_64.sse3  = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSE3 )? true : false;
		info->x86_64.ssse3 = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSSE3)? true : false;
		info->x86_64.sse41 = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSE41)? true : false;
		info->x86_64.sse42 = (flags_ecx & FLAC__CPUINFO_IA32_CPUID_SSE42)? true : false;
	}
#ifdef DEBUG
	fprintf(stderr, "CPU info (x86-64):\n");
	fprintf(stderr, "  SSE3 ....... %c\n", info->x86_64.sse3    ? 'Y' : 'n');
	fprintf(stderr, "  SSSE3 ...... %c\n", info->x86_64.ssse3   ? 'Y' : 'n');
	fprintf(stderr, "  SSE41 ...... %c\n", info->x86_64.sse41   ? 'Y' : 'n');
	fprintf(stderr, "  SSE42 ...... %c\n", info->x86_64.sse42   ? 'Y' : 'n');
#endif

#else
	info->use_asm = false;
#endif

/*
 * unknown CPU
 */
#else
	info->type = FLAC__CPUINFO_TYPE_UNKNOWN;
	info->use_asm = false;
#endif
}

#if (defined FLAC__CPU_IA32 || defined FLAC__CPU_X86_64) && defined FLAC__HAS_X86INTRIN

#if defined _MSC_VER
#include <intrin.h> /* for __cpuid() */
#elif defined __GNUC__ && defined HAVE_CPUID_H
#include <cpuid.h> /* for __get_cpuid() and __get_cpuid_max() */
#endif

FLAC__uint32 FLAC__cpu_have_cpuid_x86(void)
{
#ifdef FLAC__CPU_X86_64
	return 1;
#else
# if defined _MSC_VER || defined __INTEL_COMPILER /* Do they support CPUs w/o CPUID support (or OSes that work on those CPUs)? */
	FLAC__uint32 flags1, flags2;
	__asm {
		pushfd
		pushfd
		pop		eax
		mov		flags1, eax
		xor		eax, 0x200000
		push	eax
		popfd
		pushfd
		pop		eax
		mov		flags2, eax
		popfd
	}
	if (((flags1^flags2) & 0x200000) != 0)
		return 1;
	else
		return 0;
# elif defined __GNUC__ && defined HAVE_CPUID_H
	if (__get_cpuid_max(0, 0) != 0)
		return 1;
	else
		return 0;
# else
	return 0;
# endif
#endif
}

void FLAC__cpu_info_x86(FLAC__uint32 *flags_edx, FLAC__uint32 *flags_ecx)
{
#if defined _MSC_VER || defined __INTEL_COMPILER
	int cpuinfo[4];
	__cpuid(cpuinfo, 0);
	if(cpuinfo[0] < 1) {
		*flags_ecx = *flags_edx = 0;
		return;
	}
	__cpuid(cpuinfo, 1);
	*flags_ecx = cpuinfo[2];
	*flags_edx = cpuinfo[3];
#elif defined __GNUC__ && defined HAVE_CPUID_H
	FLAC__uint32 flags_eax, flags_ebx;
	if (0 == __get_cpuid(1, &flags_eax, &flags_ebx, flags_ecx, flags_edx))
		*flags_ecx = *flags_edx = 0;
#else
	*flags_ecx = *flags_edx = 0;
#endif
}

#endif /* (FLAC__CPU_IA32 || FLAC__CPU_X86_64) && FLAC__HAS_X86INTRIN */
