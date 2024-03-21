/**
 *  @file       simsimd.h
 *  @brief      SIMD-accelerated Similarity Measures and Distance Functions.
 *  @author     Ash Vardanian
 *  @date       March 14, 2023
 *  @copyright  Copyright (c) 2023
 *
 *  References:
 *  x86 intrinsics: https://www.intel.com/content/www/us/en/docs/intrinsics-guide/
 *  Arm intrinsics: https://developer.arm.com/architectures/instruction-sets/intrinsics/
 *  Detecting target CPU features at compile time: https://stackoverflow.com/a/28939692/2766161
 */

#ifndef SIMSIMD_H
#define SIMSIMD_H

#define SIMSIMD_VERSION_MAJOR 4
#define SIMSIMD_VERSION_MINOR 0
#define SIMSIMD_VERSION_PATCH 0

#include "binary.h"      // Hamming, Jaccard
#include "dot.h"         // Inner (dot) product, and its conjugate
#include "geospatial.h"  // Haversine and Vincenty
#include "probability.h" // Kullback-Leibler, Jensen–Shannon
#include "spatial.h"     // L2, Cosine

/**
 *  @brief  Removes compile-time dispatching, and replaces it with runtime dispatching.
 *          So the `simsimd_dot_f32` function will invoke the most advanced backend supported by the CPU,
 *          that runs the program, rather than the most advanced backend supported by the CPU
 *          used to compile the library or the downstream application.
 */
#ifndef SIMSIMD_DYNAMIC_DISPATCH
#define SIMSIMD_DYNAMIC_DISPATCH (0) // true or false
#endif

/*  Annotation for the public API symbols:
 *
 *  - `SIMSIMD_PUBLIC` is used for functions that are part of the public API.
 *  - `SIMSIMD_INTERNAL` is used for internal helper functions with unstable APIs.
 *  - `SIMSIMD_DYNAMIC` is used for functions that are part of the public API, but are dispatched at runtime.
 */
#ifndef SIMSIMD_DYNAMIC
#if SIMSIMD_DYNAMIC_DISPATCH
#if defined(_WIN32) || defined(__CYGWIN__)
#define SIMSIMD_DYNAMIC __declspec(dllexport)
#define SIMSIMD_PUBLIC inline static
#define SIMSIMD_INTERNAL inline static
#else
#define SIMSIMD_DYNAMIC __attribute__((visibility("default")))
#define SIMSIMD_PUBLIC __attribute__((unused)) inline static
#define SIMSIMD_INTERNAL __attribute__((always_inline)) inline static
#endif // _WIN32 || __CYGWIN__
#else
#define SIMSIMD_DYNAMIC inline static
#define SIMSIMD_PUBLIC inline static
#define SIMSIMD_INTERNAL inline static
#endif // SIMSIMD_DYNAMIC_DISPATCH
#endif // SIMSIMD_DYNAMIC

#if SIMSIMD_TARGET_ARM
#ifdef __linux__
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief  Enumeration of supported metric kinds.
 *          Some have aliases for convenience.
 */
typedef enum {
    simsimd_metric_unknown_k = 0, ///< Unknown metric kind

    // Classics:
    simsimd_metric_dot_k = 'i',   ///< Inner product
    simsimd_metric_inner_k = 'i', ///< Inner product alias

    simsimd_metric_vdot_k = 'v', ///< Complex inner product

    simsimd_metric_cos_k = 'c',     ///< Cosine similarity
    simsimd_metric_cosine_k = 'c',  ///< Cosine similarity alias
    simsimd_metric_angular_k = 'c', ///< Cosine similarity alias

    simsimd_metric_l2sq_k = 'e',        ///< Squared Euclidean distance
    simsimd_metric_sqeuclidean_k = 'e', ///< Squared Euclidean distance alias

    // Binary:
    simsimd_metric_hamming_k = 'h',   ///< Hamming distance
    simsimd_metric_manhattan_k = 'h', ///< Manhattan distance is same as Hamming

    simsimd_metric_jaccard_k = 'j',  ///< Jaccard coefficient
    simsimd_metric_tanimoto_k = 'j', ///< Tanimoto coefficient is same as Jaccard

    // Probability:
    simsimd_metric_kl_k = 'k',               ///< Kullback-Leibler divergence
    simsimd_metric_kullback_leibler_k = 'k', ///< Kullback-Leibler divergence alias

    simsimd_metric_js_k = 's',             ///< Jensen-Shannon divergence
    simsimd_metric_jensen_shannon_k = 's', ///< Jensen-Shannon divergence alias

} simsimd_metric_kind_t;

/**
 *  @brief  Enumeration of SIMD capabilities of the target architecture.
 */
typedef enum {
    simsimd_cap_serial_k = 1,       ///< Serial (non-SIMD) capability
    simsimd_cap_any_k = 0x7FFFFFFF, ///< Mask representing any capability with `INT_MAX`

    simsimd_cap_neon_k = 1 << 10, ///< ARM NEON capability
    simsimd_cap_sve_k = 1 << 11,  ///< ARM SVE capability
    simsimd_cap_sve2_k = 1 << 12, ///< ARM SVE2 capability

    simsimd_cap_haswell_k = 1 << 20,  ///< x86 AVX2 capability with FMA and F16C extensions
    simsimd_cap_skylake_k = 1 << 21,  ///< x86 AVX512 baseline capability
    simsimd_cap_ice_k = 1 << 22,      ///< x86 AVX512 capability with advanced integer algos
    simsimd_cap_sapphire_k = 1 << 23, ///< x86 AVX512 capability with `f16` support

} simsimd_capability_t;

/**
 *  @brief  Enumeration of supported data types.
 *
 *  Includes complex type descriptors which in C code would use the real counterparts,
 *  but the independent flags contain metadata to be passed between programming language
 *  interfaces.
 */
typedef enum {
    simsimd_datatype_unknown_k, ///< Unknown data type
    simsimd_datatype_f64_k,     ///< Double precision floating point
    simsimd_datatype_f32_k,     ///< Single precision floating point
    simsimd_datatype_f16_k,     ///< Half precision floating point
    simsimd_datatype_i8_k,      ///< 8-bit integer
    simsimd_datatype_b8_k,      ///< Single-bit values packed into 8-bit words
    simsimd_datatype_f64c_k,    ///< Complex double precision floating point
    simsimd_datatype_f32c_k,    ///< Complex single precision floating point
    simsimd_datatype_f16c_k,    ///< Complex half precision floating point
    simsimd_datatype_i8c_k,     ///< Complex 8-bit integer
} simsimd_datatype_t;

/**
 *  @brief  Type-punned function pointer accepting two vectors and outputting their similarity/distance.
 *
 *  @param[in] a Pointer to the first data array.
 *  @param[in] b Pointer to the second data array.
 *  @param[in] n Number of scalar words in the input arrays.
 *  @param[out] result Output distance as a double-precision float.
 */
typedef void (*simsimd_metric_punned_t)(void const* a, void const* b, simsimd_size_t size_a,
                                        simsimd_distance_t* result);

/**
 *  @brief  Function to determine the SIMD capabilities of the current machine at @b runtime.
 *  @return A bitmask of the SIMD capabilities represented as a `simsimd_capability_t` enum value.
 */
SIMSIMD_PUBLIC simsimd_capability_t simsimd_capabilities(void) {

#if SIMSIMD_TARGET_X86

    /// The states of 4 registers populated for a specific "cpuid" assembly call
    union four_registers_t {
        int array[4];
        struct separate_t {
            unsigned eax, ebx, ecx, edx;
        } named;
    } info1, info7;

#ifdef _MSC_VER
    __cpuidex(info1.array, 1, 0);
    __cpuidex(info7.array, 7, 0);
#else
    __asm__ __volatile__("cpuid"
                         : "=a"(info1.named.eax), "=b"(info1.named.ebx), "=c"(info1.named.ecx), "=d"(info1.named.edx)
                         : "a"(1), "c"(0));
    __asm__ __volatile__("cpuid"
                         : "=a"(info7.named.eax), "=b"(info7.named.ebx), "=c"(info7.named.ecx), "=d"(info7.named.edx)
                         : "a"(7), "c"(0));
#endif

    // Check for AVX2 (Function ID 7, EBX register)
    // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L148
    unsigned supports_avx2 = (info7.named.ebx & 0x00000020) != 0;
    // Check for F16C (Function ID 1, ECX register)
    // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L107
    unsigned supports_f16c = (info1.named.ecx & 0x20000000) != 0;
    unsigned supports_fma = (info1.named.ecx & 0x00001000) != 0;
    // Check for AVX512F (Function ID 7, EBX register)
    // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L155
    unsigned supports_avx512f = (info7.named.ebx & 0x00010000) != 0;
    unsigned supports_avx512ifma = (info7.named.ebx & 0x00200000) != 0;
    // Check for AVX512FP16 (Function ID 7, EDX register)
    // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L198C9-L198C23
    unsigned supports_avx512fp16 = (info7.named.edx & 0x00800000) != 0;
    // Check for VPOPCNTDQ (Function ID 1, ECX register)
    // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L182C30-L182C40
    unsigned supports_avx512vpopcntdq = (info1.named.ecx & 0x00004000) != 0;
    unsigned supports_avx512vbmi2 = (info1.named.ecx & 0x00000040) != 0;
    // Check for VNNI (Function ID 1, ECX register)
    // https://github.com/llvm/llvm-project/blob/50598f0ff44f3a4e75706f8c53f3380fe7faa896/clang/lib/Headers/cpuid.h#L180
    unsigned supports_avx512vnni = (info1.named.ecx & 0x00000800) != 0;
    unsigned supports_avx512bitalg = (info1.named.ecx & 0x00001000) != 0;

    // Convert specific features into CPU generations
    unsigned supports_haswell = supports_avx2 && supports_f16c && supports_fma;
    unsigned supports_skylake = supports_avx512f;
    unsigned supports_ice = supports_avx512vnni && supports_avx512ifma && supports_avx512bitalg &&
                            supports_avx512vbmi2 && supports_avx512vpopcntdq;
    unsigned supports_sapphire = supports_avx512fp16;

    return (simsimd_capability_t)(                     //
        (simsimd_cap_haswell_k * supports_haswell) |   //
        (simsimd_cap_skylake_k * supports_skylake) |   //
        (simsimd_cap_ice_k * supports_ice) |           //
        (simsimd_cap_sapphire_k * supports_sapphire) | //
        (simsimd_cap_serial_k));

#endif // SIMSIMD_TARGET_X86

#if SIMSIMD_TARGET_ARM

    // Every 64-bit Arm CPU supports NEON
    unsigned supports_neon = 1;
    unsigned supports_sve = 0;
    unsigned supports_sve2 = 0;

#ifdef __linux__
    unsigned long hwcap = getauxval(AT_HWCAP);
    unsigned long hwcap2 = getauxval(AT_HWCAP2);
    supports_sve = (hwcap & HWCAP_SVE) != 0;
    supports_sve2 = (hwcap2 & HWCAP2_SVE2) != 0;
#endif

    return (simsimd_capability_t)(             //
        (simsimd_cap_neon_k * supports_neon) | //
        (simsimd_cap_sve_k * supports_sve) |   //
        (simsimd_cap_sve2_k * supports_sve2) | //
        (simsimd_cap_serial_k));

#endif // SIMSIMD_TARGET_ARM

    return simsimd_cap_serial_k;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type"

/**
 *  @brief  Determines the best suited metric implementation based on the given datatype,
 *          supported and allowed by hardware capabilities.
 *
 *  @param kind The kind of metric to be evaluated.
 *  @param datatype The data type for which the metric needs to be evaluated.
 *  @param supported The hardware capabilities supported by the CPU.
 *  @param allowed The hardware capabilities allowed for use.
 *  @param metric_output Output variable for the selected similarity function.
 *  @param capability_output Output variable for the utilized hardware capabilities.
 */
SIMSIMD_PUBLIC void simsimd_find_metric_punned( //
    simsimd_metric_kind_t kind,                 //
    simsimd_datatype_t datatype,                //
    simsimd_capability_t supported,             //
    simsimd_capability_t allowed,               //
    simsimd_metric_punned_t* metric_output,     //
    simsimd_capability_t* capability_output) {

    simsimd_metric_punned_t* m = metric_output;
    simsimd_capability_t* c = capability_output;
    simsimd_capability_t viable = (simsimd_capability_t)(supported & allowed);
    *m = (simsimd_metric_punned_t)0;
    *c = (simsimd_capability_t)0;

    // clang-format off
    switch (datatype) {

    case simsimd_datatype_unknown_k: break;

    // Double-precision floating-point vectors
    case simsimd_datatype_f64_k:

    #if SIMSIMD_TARGET_SKYLAKE
        if (viable & simsimd_cap_skylake_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f64_skylake, *c = simsimd_cap_skylake_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f64_skylake, *c = simsimd_cap_skylake_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f64_skylake, *c = simsimd_cap_skylake_k; return;
            default: break;
            }
    #endif
        if (viable & simsimd_cap_serial_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f64_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f64_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f64_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_js_k: *m = (simsimd_metric_punned_t)&simsimd_js_f64_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_kl_k: *m = (simsimd_metric_punned_t)&simsimd_kl_f64_serial, *c = simsimd_cap_serial_k; return;
            default: break;
            }

        break;

    // Single-precision floating-point vectors
    case simsimd_datatype_f32_k:

    #if SIMSIMD_TARGET_NEON
        if (viable & simsimd_cap_neon_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f32_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f32_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f32_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_js_k: *m = (simsimd_metric_punned_t)&simsimd_js_f32_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_kl_k: *m = (simsimd_metric_punned_t)&simsimd_kl_f32_neon, *c = simsimd_cap_neon_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_SKYLAKE
        if (viable & simsimd_cap_skylake_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f32_skylake, *c = simsimd_cap_skylake_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f32_skylake, *c = simsimd_cap_skylake_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f32_skylake, *c = simsimd_cap_skylake_k; return;
            case simsimd_metric_js_k: *m = (simsimd_metric_punned_t)&simsimd_js_f32_skylake, *c = simsimd_cap_skylake_k; return;
            case simsimd_metric_kl_k: *m = (simsimd_metric_punned_t)&simsimd_kl_f32_skylake, *c = simsimd_cap_skylake_k; return;
            default: break;
            }
    #endif
        if (viable & simsimd_cap_serial_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f32_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f32_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f32_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_js_k: *m = (simsimd_metric_punned_t)&simsimd_js_f32_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_kl_k: *m = (simsimd_metric_punned_t)&simsimd_kl_f32_serial, *c = simsimd_cap_serial_k; return;
            default: break;
            }

        break;

    // Half-precision floating-point vectors
    case simsimd_datatype_f16_k:

    #if SIMSIMD_TARGET_SVE
        if (viable & simsimd_cap_sve_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16_sve, *c = simsimd_cap_sve_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f16_sve, *c = simsimd_cap_sve_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f16_sve, *c = simsimd_cap_sve_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_NEON
        if (viable & simsimd_cap_neon_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f16_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f16_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_js_k: *m = (simsimd_metric_punned_t)&simsimd_js_f16_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_kl_k: *m = (simsimd_metric_punned_t)&simsimd_kl_f16_neon, *c = simsimd_cap_neon_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_SAPPHIRE
        if (viable & simsimd_cap_sapphire_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16_sapphire, *c = simsimd_cap_sapphire_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f16_sapphire, *c = simsimd_cap_sapphire_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f16_sapphire, *c = simsimd_cap_sapphire_k; return;
            case simsimd_metric_js_k: *m = (simsimd_metric_punned_t)&simsimd_js_f16_sapphire, *c = simsimd_cap_sapphire_k; return;
            case simsimd_metric_kl_k: *m = (simsimd_metric_punned_t)&simsimd_kl_f16_sapphire, *c = simsimd_cap_sapphire_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_HASWELL
        if (viable & simsimd_cap_haswell_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f16_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f16_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_js_k: *m = (simsimd_metric_punned_t)&simsimd_js_f16_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_kl_k: *m = (simsimd_metric_punned_t)&simsimd_kl_f16_haswell, *c = simsimd_cap_haswell_k; return;
            default: break;
            }
    #endif

        if (viable & simsimd_cap_serial_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_f16_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_f16_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_js_k: *m = (simsimd_metric_punned_t)&simsimd_js_f16_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_kl_k: *m = (simsimd_metric_punned_t)&simsimd_kl_f16_serial, *c = simsimd_cap_serial_k; return;
            default: break;
            }
        
        break;

    // Single-byte integer vectors
    case simsimd_datatype_i8_k:
    #if SIMSIMD_TARGET_NEON
        if (viable & simsimd_cap_neon_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_cos_i8_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_i8_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_i8_neon, *c = simsimd_cap_neon_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_ICE
        if (viable & simsimd_cap_ice_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_cos_i8_ice, *c = simsimd_cap_ice_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_i8_ice, *c = simsimd_cap_ice_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_i8_ice, *c = simsimd_cap_ice_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_HASWELL
        if (viable & simsimd_cap_haswell_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_cos_i8_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_i8_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_i8_haswell, *c = simsimd_cap_haswell_k; return;
            default: break;
            }
    #endif

        if (viable & simsimd_cap_serial_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_cos_i8_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_cos_k: *m = (simsimd_metric_punned_t)&simsimd_cos_i8_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_l2sq_k: *m = (simsimd_metric_punned_t)&simsimd_l2sq_i8_serial, *c = simsimd_cap_serial_k; return;
            default: break;
            }
        
        break;

    // Binary vectors
    case simsimd_datatype_b8_k:

    #if SIMSIMD_TARGET_SVE
        if (viable & simsimd_cap_sve_k)
            switch (kind) {
            case simsimd_metric_hamming_k: *m = (simsimd_metric_punned_t)&simsimd_hamming_b8_sve, *c = simsimd_cap_sve_k; return;
            case simsimd_metric_jaccard_k: *m = (simsimd_metric_punned_t)&simsimd_jaccard_b8_sve, *c = simsimd_cap_sve_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_NEON
        if (viable & simsimd_cap_neon_k)
            switch (kind) {
            case simsimd_metric_hamming_k: *m = (simsimd_metric_punned_t)&simsimd_hamming_b8_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_jaccard_k: *m = (simsimd_metric_punned_t)&simsimd_jaccard_b8_neon, *c = simsimd_cap_neon_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_ICE
        if (viable & simsimd_cap_ice_k)
            switch (kind) {
            case simsimd_metric_hamming_k: *m = (simsimd_metric_punned_t)&simsimd_hamming_b8_ice, *c = simsimd_cap_ice_k; return;
            case simsimd_metric_jaccard_k: *m = (simsimd_metric_punned_t)&simsimd_jaccard_b8_ice, *c = simsimd_cap_ice_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_HASWELL
        if (viable & simsimd_cap_haswell_k)
            switch (kind) {
            case simsimd_metric_hamming_k: *m = (simsimd_metric_punned_t)&simsimd_hamming_b8_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_jaccard_k: *m = (simsimd_metric_punned_t)&simsimd_jaccard_b8_haswell, *c = simsimd_cap_haswell_k; return;
            default: break;
            }
    #endif

        if (viable & simsimd_cap_serial_k)
            switch (kind) {
            case simsimd_metric_hamming_k: *m = (simsimd_metric_punned_t)&simsimd_hamming_b8_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_jaccard_k: *m = (simsimd_metric_punned_t)&simsimd_jaccard_b8_serial, *c = simsimd_cap_serial_k; return;
            default: break;
            }
        
        break;

    case simsimd_datatype_f32c_k:

    #if SIMSIMD_TARGET_SVE
        if (viable & simsimd_cap_sve_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f32c_sve, *c = simsimd_cap_sve_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f32c_sve, *c = simsimd_cap_sve_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_NEON
        if (viable & simsimd_cap_neon_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f32c_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f32c_neon, *c = simsimd_cap_neon_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_HASWELL
        if (viable & simsimd_cap_haswell_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f32c_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f32c_haswell, *c = simsimd_cap_haswell_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_SKYLAKE
        if (viable & simsimd_cap_skylake_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f32c_skylake, *c = simsimd_cap_skylake_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f32c_skylake, *c = simsimd_cap_skylake_k; return;
            default: break;
            }
    #endif

        if (viable & simsimd_cap_serial_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f32c_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f32c_serial, *c = simsimd_cap_serial_k; return;
            default: break;
            }

    case simsimd_datatype_f64c_k:

    #if SIMSIMD_TARGET_SVE
        if (viable & simsimd_cap_sve_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f64c_sve, *c = simsimd_cap_sve_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f64c_sve, *c = simsimd_cap_sve_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_SKYLAKE
        if (viable & simsimd_cap_skylake_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f64c_skylake, *c = simsimd_cap_skylake_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f64c_skylake, *c = simsimd_cap_skylake_k; return;
            default: break;
            }
    #endif

        if (viable & simsimd_cap_serial_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f64c_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f64c_serial, *c = simsimd_cap_serial_k; return;
            default: break;
            }
        
        break;

    case simsimd_datatype_f16c_k:

    #if SIMSIMD_TARGET_SVE
        if (viable & simsimd_cap_sve_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16c_sve, *c = simsimd_cap_sve_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f16c_sve, *c = simsimd_cap_sve_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_NEON
        if (viable & simsimd_cap_neon_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16c_neon, *c = simsimd_cap_neon_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f16c_neon, *c = simsimd_cap_neon_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_HASWELL
        if (viable & simsimd_cap_haswell_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16c_haswell, *c = simsimd_cap_haswell_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f16c_haswell, *c = simsimd_cap_haswell_k; return;
            default: break;
            }
    #endif
    #if SIMSIMD_TARGET_SAPPHIRE
        if (viable & simsimd_cap_sapphire_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16c_sapphire, *c = simsimd_cap_sapphire_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f16c_sapphire, *c = simsimd_cap_sapphire_k; return;
            default: break;
            }
    #endif

        if (viable & simsimd_cap_serial_k)
            switch (kind) {
            case simsimd_metric_dot_k: *m = (simsimd_metric_punned_t)&simsimd_dot_f16c_serial, *c = simsimd_cap_serial_k; return;
            case simsimd_metric_vdot_k: *m = (simsimd_metric_punned_t)&simsimd_vdot_f16c_serial, *c = simsimd_cap_serial_k; return;
            default: break;
            }
        
        break;
    }
    // clang-format on
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop

/**
 *  @brief  Selects the most suitable metric implementation based on the given metric kind, datatype,
 *          and allowed capabilities. @b Don't call too often and prefer caching the `simsimd_capabilities()`.
 *
 *  @param kind The kind of metric to be evaluated.
 *  @param datatype The data type for which the metric needs to be evaluated.
 *  @param allowed The hardware capabilities allowed for use.
 *  @return A function pointer to the selected metric implementation.
 */
SIMSIMD_PUBLIC simsimd_metric_punned_t simsimd_metric_punned( //
    simsimd_metric_kind_t kind,                               //
    simsimd_datatype_t datatype,                              //
    simsimd_capability_t allowed) {

    simsimd_metric_punned_t result = 0;
    simsimd_capability_t c = simsimd_cap_serial_k;
    simsimd_capability_t supported = simsimd_capabilities();
    simsimd_find_metric_punned(kind, datatype, supported, allowed, &result, &c);
    return result;
}

#if SIMSIMD_DYNAMIC_DISPATCH

/*  Inner products
 *  - Dot product: the sum of the products of the corresponding elements of two vectors.
 *  - Complex Dot product: dot product with a conjugate first argument.
 *  - Complex Conjugate Dot product: dot product with a conjugate first argument.
 *
 *  @param a The first vector.
 *  @param b The second vector.
 *  @param n The number of elements in the vectors. Even for complex variants.
 *  @param d The output distance value.
 *
 *  @note The dot product can be negative, to use as a distance, take `1 - a * b`.
 *  @note The dot product is zero if and only if the two vectors are orthogonal.
 *  @note Defined only for floating-point and integer data types.
 */
SIMSIMD_DYNAMIC void simsimd_dot_f16(simsimd_f16_t const* a, simsimd_f16_t const* b, simsimd_size_t n,
                                     simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_dot_f32(simsimd_f32_t const* a, simsimd_f32_t const* b, simsimd_size_t n,
                                     simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_dot_f64(simsimd_f64_t const* a, simsimd_f64_t const* b, simsimd_size_t n,
                                     simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_dot_f16c(simsimd_f16_t const* a, simsimd_f16_t const* b, simsimd_size_t n,
                                      simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_dot_f32c(simsimd_f32_t const* a, simsimd_f32_t const* b, simsimd_size_t n,
                                      simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_dot_f64c(simsimd_f64_t const* a, simsimd_f64_t const* b, simsimd_size_t n,
                                      simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_vdot_f16c(simsimd_f16_t const* a, simsimd_f16_t const* b, simsimd_size_t n,
                                       simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_vdot_f32c(simsimd_f32_t const* a, simsimd_f32_t const* b, simsimd_size_t n,
                                       simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_vdot_f64c(simsimd_f64_t const* a, simsimd_f64_t const* b, simsimd_size_t n,
                                       simsimd_distance_t* d);

/*  Spatial distances
 *  - Cosine distance: the cosine of the angle between two vectors.
 *  - L2 squared distance: the squared Euclidean distance between two vectors.
 *
 *  @param a The first vector.
 *  @param b The second vector.
 *  @param n The number of elements in the vectors.
 *  @param d The output distance value.
 *
 *  @note The output distance value is non-negative.
 *  @note The output distance value is zero if and only if the two vectors are identical.
 *  @note Defined only for floating-point and integer data types.
 */
SIMSIMD_DYNAMIC void simsimd_cos_i8(simsimd_i8_t const* a, simsimd_i8_t const* b, simsimd_size_t c,
                                    simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_cos_f16(simsimd_f16_t const* a, simsimd_f16_t const* b, simsimd_size_t c,
                                     simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_cos_f32(simsimd_f32_t const* a, simsimd_f32_t const* b, simsimd_size_t c,
                                     simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_cos_f64(simsimd_f64_t const* a, simsimd_f64_t const* b, simsimd_size_t c,
                                     simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_l2sq_i8(simsimd_i8_t const* a, simsimd_i8_t const* b, simsimd_size_t c,
                                     simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_l2sq_f16(simsimd_f16_t const* a, simsimd_f16_t const* b, simsimd_size_t c,
                                      simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_l2sq_f32(simsimd_f32_t const* a, simsimd_f32_t const* b, simsimd_size_t c,
                                      simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_l2sq_f64(simsimd_f64_t const* a, simsimd_f64_t const* b, simsimd_size_t c,
                                      simsimd_distance_t* d);

/*  Binary distances
 *  - Hamming distance: the number of positions at which the corresponding bits are different.
 *  - Jaccard distance: ratio of bit-level matching positions (intersection) to the total number of positions (union).
 *
 *  @param a The first binary vector.
 *  @param b The second binary vector.
 *  @param n The number of 8-bit words in the vectors.
 *  @param d The output distance value.
 *
 *  @note The output distance value is non-negative.
 *  @note The output distance value is zero if and only if the two vectors are identical.
 *  @note Defined only for binary data.
 */
SIMSIMD_DYNAMIC void simsimd_hamming_b8(simsimd_b8_t const* a, simsimd_b8_t const* b, simsimd_size_t n,
                                        simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_jaccard_b8(simsimd_b8_t const* a, simsimd_b8_t const* b, simsimd_size_t n,
                                        simsimd_distance_t* d);

/*  Probability distributions
 *  - Jensen-Shannon divergence: a measure of similarity between two probability distributions.
 *  - Kullback-Leibler divergence: a measure of how one probability distribution diverges from a second.
 *
 *  @param a The first descrete probability distribution.
 *  @param b The second descrete probability distribution.
 *  @param n The number of elements in the descrete distributions.
 *  @param d The output divergence value.
 *
 *  @note The distributions are assumed to be normalized.
 *  @note The output divergence value is non-negative.
 *  @note The output divergence value is zero if and only if the two distributions are identical.
 *  @note Defined only for floating-point data types.
 */
SIMSIMD_DYNAMIC void simsimd_kl_f16(simsimd_f16_t const* a, simsimd_f16_t const* b, simsimd_size_t n,
                                    simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_kl_f32(simsimd_f32_t const* a, simsimd_f32_t const* b, simsimd_size_t n,
                                    simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_kl_f64(simsimd_f64_t const* a, simsimd_f64_t const* b, simsimd_size_t n,
                                    simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_js_f16(simsimd_f16_t const* a, simsimd_f16_t const* b, simsimd_size_t n,
                                    simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_js_f32(simsimd_f32_t const* a, simsimd_f32_t const* b, simsimd_size_t n,
                                    simsimd_distance_t* d);
SIMSIMD_DYNAMIC void simsimd_js_f64(simsimd_f64_t const* a, simsimd_f64_t const* b, simsimd_size_t n,
                                    simsimd_distance_t* d);

#endif

#ifdef __cplusplus
}
#endif

#endif
