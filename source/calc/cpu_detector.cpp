/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "pch.h"

namespace cpu
{
#ifndef MODE64
#ifdef _M_X64
#define MODE64
#endif
#endif

#if defined(__GNUC__) && __GNUC__
#ifdef MODE64
#define cpuid(func, func2, ax, bx, cx, dx)\
  __asm__ __volatile__ (\
                        "cpuid           \n\t" \
                        : "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) \
                        : "a" (func), "c" (func2));
#else
#define cpuid(func, func2, ax, bx, cx, dx)\
  __asm__ __volatile__ (\
                        "mov %%ebx, %%edi   \n\t" \
                        "cpuid              \n\t" \
                        "xchg %%edi, %%ebx  \n\t" \
                        : "=a" (ax), "=D" (bx), "=c" (cx), "=d" (dx) \
                        : "a" (func), "c" (func2));
#endif
#else
#ifdef MODE64
#if defined(_MSC_VER) && _MSC_VER > 1500
//void __cpuidex(int CPUInfo[4], int info_type, int ecxvalue);
#pragma intrinsic(__cpuidex)
#define cpuid(func, func2, a, b, c, d) do {\
    int regs[4];\
    __cpuidex(regs, func, func2); \
    a = regs[0];  b = regs[1];  c = regs[2];  d = regs[3];\
  } while(0)
#else
void __cpuid(int CPUInfo[4], int info_type);
#pragma intrinsic(__cpuid)
#define cpuid(func, func2, a, b, c, d) do {\
    int regs[4];\
    __cpuid(regs, func); \
    a = regs[0];  b = regs[1];  c = regs[2];  d = regs[3];\
  } while (0)
#endif
#else
#define cpuid(func, func2, a, b, c, d)\
  __asm mov eax, func\
  __asm mov ecx, func2\
  __asm cpuid\
  __asm mov a, eax\
  __asm mov b, ebx\
  __asm mov c, ecx\
  __asm mov d, edx
#endif
#endif /* end others */

#if !defined(__native_client__) && (defined(__i386__) || defined(__x86_64__))
    static INLINE uint64_t xgetbv(void) {
        const uint32_t ecx = 0;
        uint32_t eax, edx;
        // Use the raw opcode for xgetbv for compatibility with older toolchains.
        __asm__ volatile (
            ".byte 0x0f, 0x01, 0xd0\n"
            : "=a"(eax), "=d"(edx) : "c" (ecx));
        return ((uint64_t)edx << 32) | eax;
    }
#elif (defined(_M_X64) || defined(_M_IX86)) && \
      defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 160040219  // >= VS2010 SP1
#include <immintrin.h>
#define xgetbv() _xgetbv(0)
#elif defined(_MSC_VER) && defined(_M_IX86)
    static INLINE uint64_t xgetbv(void) {
        uint32_t eax_, edx_;
        __asm {
            xor ecx, ecx  // ecx = 0
            // Use the raw opcode for xgetbv for compatibility with older toolchains.
            __asm _emit 0x0f __asm _emit 0x01 __asm _emit 0xd0
            mov eax_, eax
            mov edx_, edx
        }
        return ((uint64_t)edx_ << 32) | eax_;
    }
#else
#define xgetbv() 0U  // no AVX for older x64 or unrecognized toolchains.
#endif


u32 detect_caps()
{
    unsigned int max_cpuid_val, reg_eax, reg_ebx, reg_ecx, reg_edx;

    /* Ensure that the CPUID instruction supports extended features */
    cpuid(0, 0, max_cpuid_val, reg_ebx, reg_ecx, reg_edx);

    if (max_cpuid_val < 1)
        return 0;

    u32 flags = 0;

    /* Get the standard feature flags */
    cpuid(1, 0, reg_eax, reg_ebx, reg_ecx, reg_edx);

    if (reg_edx & ONEBIT(23)) flags |= CPU_MMX;
    if (reg_edx & ONEBIT(25)) flags |= CPU_SSE; /* aka xmm */
    if (reg_edx & ONEBIT(26)) flags |= CPU_SSE2; /* aka wmt */
    if (reg_ecx & ONEBIT(0)) flags |= CPU_SSE3;
    if (reg_ecx & ONEBIT(9)) flags |= CPU_SSSE3;
    if (reg_ecx & ONEBIT(19)) flags |= CPU_SSE41;

    // bits 27 (OSXSAVE) & 28 (256-bit AVX)
    if ((reg_ecx & (ONEBIT(27) | ONEBIT(28))) == (ONEBIT(27) | ONEBIT(28))) {
        if ((xgetbv() & 0x6) == 0x6) {
            flags |= CPU_AVX;

            if (max_cpuid_val >= 7) {
                /* Get the leaf 7 feature flags. Needed to check for AVX2 support */
                cpuid(7, 0, reg_eax, reg_ebx, reg_ecx, reg_edx);

                if (reg_ebx & ONEBIT(5)) flags |= CPU_AVX2;
            }
        }
    }


    return flags;
}
int detect_cores()
{
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    return sysinfo.dwNumberOfProcessors;
#elif defined __linux__
    return sysconf( _SC_NPROCESSORS_ONLN );
#else
    --- error: copy code from http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
#endif
}
}

extern "C"
{
    unsigned int g_cpu_caps = cpu::detect_caps();
    int g_cpu_cores = cpu::detect_cores();
}

