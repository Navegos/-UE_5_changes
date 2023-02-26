// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if (defined(PLATFORM_ALWAYS_HAS_AVX) && (PLATFORM_ALWAYS_HAS_AVX > 0)) && !defined(RL_BUILD_WITH_AVX)
#define RL_BUILD_WITH_AVX
#endif  // RL_BUILD_WITH_AVX

#if (defined(PLATFORM_ALWAYS_HAS_SSE4_2) && (PLATFORM_ALWAYS_HAS_SSE4_2 > 0)) && !defined(RL_BUILD_WITH_SSE)
#define RL_BUILD_WITH_SSE
#endif  // RL_BUILD_WITH_SSE

#if (defined(PLATFORM_ALWAYS_HAS_F16C) && (PLATFORM_ALWAYS_HAS_F16C > 0)) && !defined(RL_USE_HALF_FLOATS) && !defined(RL_BUILD_WITH_AVX) && !defined(RL_BUILD_WITH_SSE)
#define RL_USE_HALF_FLOATS
#endif  // RL_USE_HALF_FLOATS

#if defined(RL_AUTODETECT_SSE) && !defined(RL_BUILD_WITH_SSE)
    // Assume any x86 architecture we try to build on will support SSE
    #if defined(i386) || defined(__i386) || defined(__i386__) || defined(__IA32__) || defined(_M_IX86) || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
        #define RL_BUILD_WITH_SSE 1
    #endif
#endif  // RL_AUTODETECT_SSE
