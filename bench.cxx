#include <thread>

#include <benchmark/benchmark.h>

#include <simsimd/simsimd.h>

namespace bm = benchmark;

static const std::size_t threads_k = std::thread::hardware_concurrency();
static const std::size_t time_k = 10;

template <typename scalar_at, typename metric_at, std::size_t bytes_per_vector_ak = 256,
          std::size_t dimensions_ak = bytes_per_vector_ak / sizeof(scalar_at)> //
static void measure(bm::State& state, metric_at metric) {

    alignas(64) scalar_at a[dimensions_ak]{};
    alignas(64) scalar_at b[dimensions_ak]{};
    float c{};

    std::fill_n(a, dimensions_ak, static_cast<scalar_at>(1));
    std::fill_n(b, dimensions_ak, static_cast<scalar_at>(2));

    for (auto _ : state)
        bm::DoNotOptimize((c = metric(a, b, dimensions_ak)));

    state.SetBytesProcessed(state.iterations() * bytes_per_vector_ak * 2u);
    state.SetItemsProcessed(state.iterations());
}

template <typename scalar_at, typename metric_at> void register_(char const* name, metric_at distance_func) {
    bm::RegisterBenchmark(name, measure<scalar_at, metric_at>, distance_func)->Threads(threads_k)->MinTime(time_k);
}

int main(int argc, char** argv) {

    bool compiled_with_sve = false;
    bool compiled_with_neon = false;
    bool compiled_with_avx2 = false;
    bool compiled_with_avx512popcnt = false;

#if defined(__ARM_FEATURE_SVE)
    compiled_with_sve = true;
#endif
#if defined(__ARM_NEON)
    compiled_with_neon = true;
#endif
#if defined(__AVX2__)
    compiled_withavx2 = true;
#endif
#if defined(__AVX512VPOPCNTDQ__)
    compiled_with_avx512popcnt = true;
#endif

    // Log supported functionality
    char const* flags[2] = {"false", "true"};
    std::printf("Benchmarking Similarity Measures\n");
    std::printf("\n");
    std::printf("- Arm NEON support enabled: %s\n", flags[compiled_with_neon]);
    std::printf("- Arm SVE support enabled: %s\n", flags[compiled_with_sve]);
    std::printf("- x86 AVX2 support enabled: %s\n", flags[compiled_with_avx2]);
    std::printf("- x86 AVX512VPOPCNTDQ support enabled: %s\n", flags[compiled_with_avx512popcnt]);
    std::printf("\n");

    // Run the benchmarks
    bm::Initialize(&argc, argv);
    if (bm::ReportUnrecognizedArguments(argc, argv))
        return 1;

#if defined(__ARM_FEATURE_SVE)
    register_<simsimd_f32_t>("dot_f32sve", simsimd_dot_f32sve);
    register_<simsimd_f32_t>("cos_f32sve", simsimd_cos_f32sve);
    register_<simsimd_f32_t>("l2sq_f32sve", simsimd_l2sq_f32sve);
    register_<std::int16_t>("l2sq_f16sve", simsimd_l2sq_f16sve);
    register_<std::uint8_t>("hamming_b1x8sve", simsimd_hamming_b1x8sve);
    register_<std::uint8_t>("hamming_b1x128sve", simsimd_hamming_b1x128sve);
#endif

#if defined(__ARM_NEON)
    register_<simsimd_f32_t>("dot_f32x4neon", simsimd_dot_f32x4neon);
    register_<std::int16_t>("cos_f16x4neon", simsimd_cos_f16x4neon);
    register_<std::int8_t>("cos_i8x16neon", simsimd_cos_i8x16neon);
    register_<simsimd_f32_t>("cos_f32x4neon", simsimd_cos_f32x4neon);
#endif

#if defined(__AVX2__)
    register_<simsimd_f32_t>("dot_f32x4avx2", simsimd_dot_f32x4avx2);
    register_<std::int8_t>("dot_i8x16avx2", simsimd_dot_i8x16avx2);
    register_<simsimd_f32_t>("cos_f32x4avx2", simsimd_cos_f32x4avx2);
#endif

#if defined(__AVX512F__)
    register_<std::int16_t>("cos_f16x16avx512", simsimd_cos_f16x16avx512);
    register_<std::uint8_t>("hamming_b1x128avx512", simsimd_hamming_b1x128avx512);
#endif

    bm::RunSpecifiedBenchmarks();
    bm::Shutdown();
    return 0;
}