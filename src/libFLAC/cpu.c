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
#elif defined FLAC__CPU_PPC
# if !defined FLAC__NO_ASM
#  if defined FLAC__SYS_DARWIN
#   include <sys/sysctl.h>
#   include <mach/mach.h>
#   include <mach/mach_host.h>
#   include <mach/host_info.h>
#   include <mach/machine.h>
#   ifndef CPU_SUBTYPE_POWERPC_970
#    define CPU_SUBTYPE_POWERPC_970 ((cpu_subtype_t) 100)
#   endif
#  else /* FLAC__SYS_DARWIN */

#   include <signal.h>
#   include <setjmp.h>

static sigjmp_buf jmpbuf;
static volatile sig_atomic_t canjump = 0;

static void sigill_handler (int sig)
{
	if (!canjump) {
		signal (sig, SIG_DFL);
		raise (sig);
	}
	canjump = 0;
	siglongjmp (jmpbuf, 1);
}
#  endif /* FLAC__SYS_DARWIN */
# endif /* FLAC__NO_ASM */
#endif /* FLAC__CPU_PPC */

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
#  undef USE_OBSOLETE_SIGCONTEXT_FLAVOR /* #define this to use the older signal handler method */
#  ifdef USE_OBSOLETE_SIGCONTEXT_FLAVOR
	static void sigill_handler_sse_os(int signal, struct sigcontext sc)
	{
		(void)signal;
		sc.eip += 3 + 3 + 6;
	}
#  else
#   include <sys/ucontext.h>
	static void sigill_handler_sse_os(int signal, siginfo_t *si, void *uc)
	{
		(void)signal, (void)si;
		((ucontext_t*)uc)->uc_mcontext.gregs[14/*REG_EIP*/] += 3 + 3 + 6;
	}
#  endif
# elif defined(_MSC_VER)
#  include <windows.h>
#  define USE_TRY_CATCH_FLAVOR /* sigill_handler flavor resulted in several crash reports on win32 */
#  ifdef USE_TRY_CATCH_FLAVOR
#  else
	LONG WINAPI sigill_handler_sse_os(EXCEPTION_POINTERS *ep)
	{
		if(ep->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION) {
			ep->ContextRecord->Eip += 3 + 3 + 6;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}
#  endif
# elif defined(_WIN32) && defined(__GNUC__)
#  undef USE_FXSR_FLAVOR
#  ifdef USE_FXSR_FLAVOR
  /* not guaranteed to work on some unknown future Intel CPUs */
#  else
  /* exception handler is process-wide; not good for a library */
#  include <windows.h>
	LONG WINAPI sigill_handler_sse_os(EXCEPTION_POINTERS *ep); /* to suppress GCC warning */
	LONG WINAPI sigill_handler_sse_os(EXCEPTION_POINTERS *ep)
	{
		if(ep->ExceptionRecord->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION) {
			ep->ContextRecord->Eip += 3 + 3 + 6;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		return EXCEPTION_CONTINUE_SEARCH;
	}
#  endif
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
			info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
#elif defined FLAC__SSE_OS
			/* assume user knows better than us; leave as detected above */
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__) || defined(__APPLE__)
			int sse = 0;
			size_t len;
			/* at least one of these must work: */
			len = sizeof(sse); sse = sse || (sysctlbyname("hw.instruction_sse", &sse, &len, NULL, 0) == 0 && sse);
			len = sizeof(sse); sse = sse || (sysctlbyname("hw.optional.sse"   , &sse, &len, NULL, 0) == 0 && sse); /* __APPLE__ ? */
			if(!sse)
				info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
#elif defined(__NetBSD__) || defined (__OpenBSD__)
# if __NetBSD_Version__ >= 105250000 || (defined __OpenBSD__)
			int val = 0, mib[2] = { CTL_MACHDEP, CPU_SSE };
			size_t len = sizeof(val);
			if(sysctl(mib, 2, &val, &len, NULL, 0) < 0 || !val)
				info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
			else { /* double-check SSE2 */
				mib[1] = CPU_SSE2;
				len = sizeof(val);
				if(sysctl(mib, 2, &val, &len, NULL, 0) < 0 || !val)
					info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
			}
# else
			info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
# endif
#elif defined(__linux__)
			int sse = 0;
			struct sigaction sigill_save;
#ifdef USE_OBSOLETE_SIGCONTEXT_FLAVOR
			if(0 == sigaction(SIGILL, NULL, &sigill_save) && signal(SIGILL, (void (*)(int))sigill_handler_sse_os) != SIG_ERR)
#else
			struct sigaction sigill_sse;
			sigill_sse.sa_sigaction = sigill_handler_sse_os;
			__sigemptyset(&sigill_sse.sa_mask);
			sigill_sse.sa_flags = SA_SIGINFO | SA_RESETHAND; /* SA_RESETHAND just in case our SIGILL return jump breaks, so we don't get stuck in a loop */
			if(0 == sigaction(SIGILL, &sigill_sse, &sigill_save))
#endif
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
				info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
#elif defined(_MSC_VER)
# ifdef USE_TRY_CATCH_FLAVOR
			__try {
				__asm {
					xorps xmm0,xmm0
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER) {
				if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
					info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
			}
# else
			int sse = 0;
			/* From MSDN: SetUnhandledExceptionFilter replaces the existing top-level exception filter for all threads in the calling process */
			/* So sigill_handler_sse_os() is process-wide and affects other threads as well (not a good thing for a library in a multi-threaded process) */
			LPTOP_LEVEL_EXCEPTION_FILTER save = SetUnhandledExceptionFilter(sigill_handler_sse_os);
			/* see GCC version above for explanation */
			/*  http://msdn.microsoft.com/en-us/library/4ks26t93.aspx */
			/*  http://www.codeproject.com/Articles/5267/Inline-Assembly-in-GCC-Vs-VC */
			__asm {
				xorps xmm0,xmm0
				inc sse
				nop
				nop
				nop
				nop
				nop
				nop
				nop
				nop
				nop
			}
			SetUnhandledExceptionFilter(save);
			if(!sse)
				info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
# endif
#elif defined(_WIN32) && defined(__GNUC__)
# ifdef USE_FXSR_FLAVOR
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
				info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
# else
			int sse = 0;
			LPTOP_LEVEL_EXCEPTION_FILTER save = SetUnhandledExceptionFilter(sigill_handler_sse_os);
			/* see MSVC version above for explanation */
			asm volatile (
				"xorps %%xmm0,%%xmm0\n\t"
				"incl %0\n\t"
				"nop\n\t" /* SIGILL jump lands here if "inc" is 9 bytes */
				"nop\n\t"
				"nop\n\t"
				"nop\n\t"
				"nop\n\t"
				"nop\n\t"
				"nop\n\t"
				"nop\n\t"
				"nop"     /* SIGILL jump lands here if "inc" is 1 byte  */
				: "=r"(sse)
				: "0"(sse)
			);
			SetUnhandledExceptionFilter(save);
			if(!sse)
				info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
# endif
#else
			/* no way to test, disable to be safe */
			info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
#endif
#ifdef DEBUG
			fprintf(stderr, "  SSE OS sup . %c\n", info->ia32.sse     ? 'Y' : 'n');
#endif
		}
		else /* info->ia32.sse == false */
			info->ia32.fxsr = info->ia32.sse = info->ia32.sse2 = info->ia32.sse3 = info->ia32.ssse3 = info->ia32.sse41 = info->ia32.sse42 = false;
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
 * PPC-specific
 */
#elif defined FLAC__CPU_PPC
	info->type = FLAC__CPUINFO_TYPE_PPC;
# if !defined FLAC__NO_ASM
	info->use_asm = true;
#  ifdef FLAC__USE_ALTIVEC
#   if defined FLAC__SYS_DARWIN
	{
		int val = 0, mib[2] = { CTL_HW, HW_VECTORUNIT };
		size_t len = sizeof(val);
		info->ppc.altivec = !(sysctl(mib, 2, &val, &len, NULL, 0) || !val);
	}
	{
		host_basic_info_data_t hostInfo;
		mach_msg_type_number_t infoCount;

		infoCount = HOST_BASIC_INFO_COUNT;
		host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostInfo, &infoCount);

		info->ppc.ppc64 = (hostInfo.cpu_type == CPU_TYPE_POWERPC) && (hostInfo.cpu_subtype == CPU_SUBTYPE_POWERPC_970);
	}
#   else /* FLAC__USE_ALTIVEC && !FLAC__SYS_DARWIN */
	{
		/* no Darwin, do it the brute-force way */
		/* @@@@@@ this is not thread-safe; replace with SSE OS method above or remove */
		info->ppc.altivec = 0;
		info->ppc.ppc64 = 0;

		signal (SIGILL, sigill_handler);
		canjump = 0;
		if (!sigsetjmp (jmpbuf, 1)) {
			canjump = 1;

			asm volatile (
				"mtspr 256, %0\n\t"
				"vand %%v0, %%v0, %%v0"
				:
				: "r" (-1)
			);

			info->ppc.altivec = 1;
		}
		canjump = 0;
		if (!sigsetjmp (jmpbuf, 1)) {
			int x = 0;
			canjump = 1;

			/* PPC64 hardware implements the cntlzd instruction */
			asm volatile ("cntlzd %0, %1" : "=r" (x) : "r" (x) );

			info->ppc.ppc64 = 1;
		}
		signal (SIGILL, SIG_DFL); /*@@@@@@ should save and restore old signal */
	}
#   endif
#  else /* !FLAC__USE_ALTIVEC */
	info->ppc.altivec = 0;
	info->ppc.ppc64 = 0;
#  endif
# else
	info->use_asm = false;
# endif

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
