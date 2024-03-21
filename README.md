# SimSIMD 📏

_Computing dot-products, similarity measures, and distances between low- and high-dimensional vectors is ubiquitous in Machine Learning, Scientific Computing, Geo-Spatial Analysis, and Information Retrieval.
These algorithms generally have linear complexity in time, constant complexity in space, and are data-parallel.
In other words, it is easily parallelizable and vectorizable and often available in packages like BLAS and LAPACK, as well as higher-level `numpy` and `scipy` Python libraries.
Ironically, even with decades of evolution in compilers and numerical computing, [most libraries can be 3-200x slower than hardware potential][benchmarks] even on the most popular hardware, like 64-bit x86 and Arm CPUs.
SimSIMD attempts to fill that gap.
1️⃣ SimSIMD functions are practically as fast as `memcpy`.
2️⃣ SimSIMD [compiles to more platforms than NumPy (105 vs 35)][compatibility] and has more backends than most BLAS implementations.
It is currently powering search in [USearch](https://github.com/unum-cloud/usearch) and several DBMS products._

[benchmarks]: https://ashvardanian.com/posts/simsimd-faster-scipy
[compatibility]: https://pypi.org/project/simsimd/#files

<div>
<a href="https://pepy.tech/project/simsimd">
    <img alt="PyPI" src="https://static.pepy.tech/personalized-badge/simsimd?period=total&units=abbreviation&left_color=black&right_color=blue&left_text=SimSIMD%20Python%20installs" />
</a>
<a href="https://www.npmjs.com/package/simsimd">
    <img alt="npm" src="https://img.shields.io/npm/dy/simsimd?label=JavaScript%20NPM%20installs" />
</a>
<a href="https://crates.io/crates/simsimd">
    <img alt="rust" src="https://img.shields.io/crates/d/simsimd?label=Rust%20Crate%20installs" />
</a>
<img alt="GitHub code size in bytes" src="https://img.shields.io/github/languages/code-size/ashvardanian/simsimd">
<a href="https://github.com/ashvardanian/SimSIMD/actions/workflows/release.yml">
    <img alt="GitHub Actions Ubuntu" src="https://img.shields.io/github/actions/workflow/status/ashvardanian/SimSIMD/release.yml?branch=main&label=Ubuntu&logo=github&color=blue">
</a>
<a href="https://github.com/ashvardanian/SimSIMD/actions/workflows/release.yml">
    <img alt="GitHub Actions Windows" src="https://img.shields.io/github/actions/workflow/status/ashvardanian/SimSIMD/release.yml?branch=main&label=Windows&logo=windows&color=blue">
</a>
<a href="https://github.com/ashvardanian/SimSIMD/actions/workflows/release.yml">
    <img alt="GitHub Actions MacOS" src="https://img.shields.io/github/actions/workflow/status/ashvardanian/SimSIMD/release.yml?branch=main&label=MacOS&logo=apple&color=blue">
</a>
<a href="https://github.com/ashvardanian/SimSIMD/actions/workflows/release.yml">
    <img alt="GitHub Actions CentOS Linux" src="https://img.shields.io/github/actions/workflow/status/ashvardanian/SimSIMD/release.yml?branch=main&label=CentOS&logo=centos&color=blue">
</a>

</div>

__Implemented distance functions__ include:

- Euclidean (L2) and Cosine (Angular) spatial distances for Vector Search.
- Dot-Products for real & complex vectors for DSP & Quantum computing.
- Hamming (~ Manhattan) and Jaccard (~ Tanimoto) bit-level distances.
- Kullback-Leibler and Jensen–Shannon divergences for probability distributions.
- Haversine and Vincenty's formulae for Geospatial Analysis.
- For Levenshtein, Needleman–Wunsch and other text metrics, check [StringZilla][stringzilla].

Moreover, SimSIMD...

- handles `f64`, `f32`, and `f16` real & complex vectors.
- handles `i8` integral and `b8` binary vectors.
- is a zero-dependency [header-only C 99](#using-simsimd-in-c) library.
- has bindings for [Python](#using-simsimd-in-python), [Rust](#using-simsimd-in-rust) and [JavaScript](#using-simsimd-in-javascript).
- has Arm backends for NEON and Scalable Vector Extensions (SVE).
- has x86 backends for Haswell, Skylake, Ice Lake, and Sapphire Rapids.

We enumerate subsets of AVX-512 instructions in Intel CPU generations, but they also work on AMD.

[scipy]: https://docs.scipy.org/doc/scipy/reference/spatial.distance.html#module-scipy.spatial.distance
[numpy]: https://numpy.org/doc/stable/reference/generated/numpy.inner.html
[stringzilla]: https://github.com/ashvardanian/stringzilla

__Technical Insights__ and related articles:

- [Uses Horner's method for polynomial approximations, beating GCC 12 by 119x](https://ashvardanian.com/posts/gcc-12-vs-avx512fp16/).
- [Uses Arm SVE and x86 AVX-512's masked loads to eliminate tail `for`-loops](https://ashvardanian.com/posts/simsimd-faster-scipy/#tails-of-the-past-the-significance-of-masked-loads).
- [Uses AVX-512 FP16 for half-precision operations, that few compilers vectorize](https://ashvardanian.com/posts/simsimd-faster-scipy/#the-challenge-of-f16).
- [Substitutes LibC's `sqrt` calls with bit-hacks using Jan Kadlec's constant](https://ashvardanian.com/posts/simsimd-faster-scipy/#bonus-section-bypassing-sqrt-and-libc-dependencies).
- [For Python avoids slow PyBind11, SWIG, and even `PyArg_ParseTuple` for speed](https://ashvardanian.com/posts/pybind11-cpython-tutorial/).
- [For JavaScript uses typed arrays and NAPI for zero-copy calls](https://ashvardanian.com/posts/javascript-ai-vector-search/).

## Benchmarks

### Against NumPy and SciPy

Given 1000 embeddings from OpenAI Ada API with 1536 dimensions, running on the Apple M2 Pro Arm CPU with NEON support, here's how SimSIMD performs against conventional methods:

| Kind                      | `f32` improvement | `f16` improvement | `i8` improvement | Conventional method                    | SimSIMD         |
| :------------------------ | ----------------: | ----------------: | ---------------: | :------------------------------------- | :-------------- |
| Inner Product             |           __2 x__ |           __9 x__ |         __18 x__ | `numpy.inner`                          | `inner`         |
| Cosine Distance           |          __32 x__ |          __79 x__ |        __133 x__ | `scipy.spatial.distance.cosine`        | `cosine`        |
| Euclidean Distance ²      |           __5 x__ |          __26 x__ |         __17 x__ | `scipy.spatial.distance.sqeuclidean`   | `sqeuclidean`   |
| Jensen-Shannon Divergence |          __31 x__ |          __53 x__ |                  | `scipy.spatial.distance.jensenshannon` | `jensenshannon` |

### Against GCC Auto-Vectorization

On the Intel Sapphire Rapids platform, SimSIMD was benchmarked against auto-vectorized code using GCC 12.
GCC handles single-precision `float` but might not be the best choice for `int8` and `_Float16` arrays, which have been part of the C language since 2011.

| Kind                      | GCC 12 `f32` | GCC 12 `f16` | SimSIMD `f16` | `f16` improvement |
| :------------------------ | -----------: | -----------: | ------------: | ----------------: |
| Inner Product             |    3,810 K/s |      192 K/s |     5,990 K/s |          __31 x__ |
| Cosine Distance           |    3,280 K/s |      336 K/s |     6,880 K/s |          __20 x__ |
| Euclidean Distance ²      |    4,620 K/s |      147 K/s |     5,320 K/s |          __36 x__ |
| Jensen-Shannon Divergence |    1,180 K/s |       18 K/s |     2,140 K/s |         __118 x__ |

__Broader Benchmarking Results__:

- [Apple M2 Pro](https://ashvardanian.com/posts/simsimd-faster-scipy/#appendix-1-performance-on-apple-m2-pro).
- [4th Gen Intel Xeon Platinum](https://ashvardanian.com/posts/simsimd-faster-scipy/#appendix-2-performance-on-4th-gen-intel-xeon-platinum-8480).
- [AWS Graviton 3](https://ashvardanian.com/posts/simsimd-faster-scipy/#appendix-3-performance-on-aws-graviton-3).

## Using SimSIMD in Python

The package is intended to replace the usage of `numpy.inner`, `numpy.dot`, and `scipy.spatial.distance`.
Aside from drastic performance improvements, SimSIMD significantly improves accuracy in mixed precision setups.
NumPy and SciPy, processing `i8` or `f16` vectors, will use the same types for accumulators, while SimSIMD can combine `i8` enumeration, `i16` multiplication, and `i32` accumulation to avoid overflows entirely.
The same applies to processing `f16` values with `f32` precision.

### Installation

Use the following snippet to install SimSIMD and list available hardware acceleration options available on your machine:

```sh
pip install simsimd
python -c "import simsimd; print(simsimd.get_capabilities())"
```

### One-to-One Distance

```py
import simsimd
import numpy as np

vec1 = np.random.randn(1536).astype(np.float32)
vec2 = np.random.randn(1536).astype(np.float32)
dist = simsimd.cosine(vec1, vec2)
```

Supported functions include `cosine`, `inner`, `sqeuclidean`, `hamming`, and `jaccard`.
Dot products are supported for both real and complex numbers:

```py
vec1 = np.random.randn(768).astype(np.float64) + 1j * np.random.randn(768).astype(np.float64)
vec2 = np.random.randn(768).astype(np.float64) + 1j * np.random.randn(768).astype(np.float64)

dist = simsimd.dot(vec1.astype(np.complex128), vec2.astype(np.complex128))
dist = simsimd.dot(vec1.astype(np.complex64), vec2.astype(np.complex64))
dist = simsimd.vdot(vec1.astype(np.complex64), vec2.astype(np.complex64)) # conjugate, same as `np.vdot`
```

Unlike SciPy, SimSIMD allows explicitly stating the precision of the input vectors, which is especially useful for mixed-precision setups.

```py
dist = simsimd.cosine(vec1, vec2, "i8")
dist = simsimd.cosine(vec1, vec2, "f16")
dist = simsimd.cosine(vec1, vec2, "f32")
dist = simsimd.cosine(vec1, vec2, "f64")
```

It also allows using SimSIMD for half-precision complex numbers, which NumPy does not support.
For that, view data as continuous even-length `np.float16` vectors and override type-resolution with `complex32` string.

```py
vec1 = np.random.randn(1536).astype(np.float16)
vec2 = np.random.randn(1536).astype(np.float16)
simd.dot(vec1, vec2, "complex32")
simd.vdot(vec1, vec2, "complex32")
```

### One-to-Many Distances

Every distance function can be used not only for one-to-one but also one-to-many and many-to-many distance calculations.
For one-to-many:

```py
vec1 = np.random.randn(1536).astype(np.float32) # rank 1 tensor
batch1 = np.random.randn(1, 1536).astype(np.float32) # rank 2 tensor
batch2 = np.random.randn(100, 1536).astype(np.float32)

dist_rank1 = simsimd.cosine(vec1, batch2)
dist_rank2 = simsimd.cosine(batch1, batch2)
```

### Many-to-Many Distances

All distance functions in SimSIMD can be used to compute many-to-many distances.
For two batches of 100 vectors to compute 100 distances, one would call it like this:

```py
batch1 = np.random.randn(100, 1536).astype(np.float32)
batch2 = np.random.randn(100, 1536).astype(np.float32)
dist = simsimd.cosine(batch1, batch2)
```

Input matrices must have identical shapes.
This functionality isn't natively present in NumPy or SciPy, and generally requires creating intermediate arrays, which is inefficient and memory-consuming.

### Many-to-Many All-Pairs Distances

One can use SimSIMD to compute distances between all possible pairs of rows across two matrices (akin to [`scipy.spatial.distance.cdist`](https://docs.scipy.org/doc/scipy/reference/generated/scipy.spatial.distance.cdist.html)).
The resulting object will have a type `DistancesTensor`, zero-copy compatible with NumPy and other libraries.
For two arrays of 10 and 1,000 entries, the resulting tensor will have 10,000 cells:

```py
import numpy as np
from simsimd import cdist, DistancesTensor

matrix1 = np.random.randn(1000, 1536).astype(np.float32)
matrix2 = np.random.randn(10, 1536).astype(np.float32)
distances: DistancesTensor = simsimd.cdist(matrix1, matrix2, metric="cosine") # zero-copy
distances_array: np.ndarray = np.array(distances, copy=True) # now managed by NumPy
```

### Multithreading

By default, computations use a single CPU core.
To optimize and utilize all CPU cores on Linux systems, add the `threads=0` argument.
Alternatively, specify a custom number of threads:

```py
distances = simsimd.cdist(matrix1, matrix2, metric="cosine", threads=0)
```

### Using Python API with USearch

Want to use it in Python with [USearch](https://github.com/unum-cloud/usearch)?
You can wrap the raw C function pointers SimSIMD backends into a `CompiledMetric` and pass it to USearch, similar to how it handles Numba's JIT-compiled code.

```py
from usearch.index import Index, CompiledMetric, MetricKind, MetricSignature
from simsimd import pointer_to_sqeuclidean, pointer_to_cosine, pointer_to_inner

metric = CompiledMetric(
    pointer=pointer_to_cosine("f16"),
    kind=MetricKind.Cos,
    signature=MetricSignature.ArrayArraySize,
)

index = Index(256, metric=metric)
```

## Using SimSIMD in Rust

To install, add the following to your `Cargo.toml`:

```toml
[dependencies]
simsimd = "..."
```

Before using the SimSIMD library, ensure you have imported the necessary traits and types into your Rust source file.
The library provides several traits for different distance/similarity kinds - `SpatialSimilarity`, `BinarySimilarity`, and `ProbabilitySimilarity`.

```rust
use simsimd::SpatialSimilarity;

fn main() {
    let vector_a: Vec<f32> = vec![1.0, 2.0, 3.0];
    let vector_b: Vec<f32> = vec![4.0, 5.0, 6.0];

    // Compute the cosine similarity between vector_a and vector_b
    let cosine_similarity = f32::cosine(&vector_a, &vector_b)
        .expect("Vectors must be of the same length");

    println!("Cosine Similarity: {}", cosine_similarity);

    // Compute the squared Euclidean distance between vector_a and vector_b
    let sq_euclidean_distance = f32::sqeuclidean(&vector_a, &vector_b)
        .expect("Vectors must be of the same length");

    println!("Squared Euclidean Distance: {}", sq_euclidean_distance);
}
```

Similarly, one can compute bit-level distance functions between slices of unsigned integers:

```rust
use simsimd::BinarySimilarity;

fn main() {
    let vector_a = &[0b11110000, 0b00001111, 0b10101010];
    let vector_b = &[0b11110000, 0b00001111, 0b01010101];

    // Compute the Hamming distance between vector_a and vector_b
    let hamming_distance = u8::hamming(&vector_a, &vector_b)
        .expect("Vectors must be of the same length");

    println!("Hamming Distance: {}", hamming_distance);

    // Compute the Jaccard distance between vector_a and vector_b
    let jaccard_distance = u8::jaccard(&vector_a, &vector_b)
        .expect("Vectors must be of the same length");

    println!("Jaccard Distance: {}", jaccard_distance);
}
```

Rust has no native support for half-precision floating-point numbers, but SimSIMD provides a `f16` type.
It has no functionality - it is a `transparent` wrapper around `u16` and can be used with `half` or any other half-precision library.

```rust
use simsimd::SpatialSimilarity;
use simsimd::f16 as SimF16;
use half::f16 as HalfF16;

fn main() {
    let vector_a: Vec<HalfF16> = ...
    let vector_b: Vec<HalfF16> = ...

    let buffer_a: &[SimF16] = unsafe { std::slice::from_raw_parts(a_half.as_ptr() as *const SimF16, a_half.len()) };
    let buffer_b: &[SimF16] = unsafe { std::slice::from_raw_parts(b_half.as_ptr() as *const SimF16, b_half.len()) };

    // Compute the cosine similarity between vector_a and vector_b
    let cosine_similarity = SimF16::cosine(&vector_a, &vector_b)
        .expect("Vectors must be of the same length");

    println!("Cosine Similarity: {}", cosine_similarity);
}
```

## Using SimSIMD in JavaScript

To install, choose one of the following options depending on your environment:

- `npm install --save simsimd`
- `yarn add simsimd`
- `pnpm add simsimd`
- `bun install simsimd`

The package is distributed with prebuilt binaries for Node.js v10 and above for Linux (x86_64, arm64), macOS (x86_64, arm64), and Windows (i386, x86_64).
If your platform is not supported, you can build the package from the source via `npm run build`.
This will automatically happen unless you install the package with the `--ignore-scripts` flag or use Bun.
After you install it, you will be able to call the SimSIMD functions on various `TypedArray` variants:

```js
const { sqeuclidean, cosine, inner, hamming, jaccard } = require('simsimd');

const vectorA = new Float32Array([1.0, 2.0, 3.0]);
const vectorB = new Float32Array([4.0, 5.0, 6.0]);

const distance = sqeuclidean(vectorA, vectorB);
console.log('Squared Euclidean Distance:', distance);
```

Other numeric types and precision levels are supported as well:

```js
const vectorA = new Float64Array([1.0, 2.0, 3.0]);
const vectorB = new Float64Array([4.0, 5.0, 6.0]);

const distance = cosine(vectorA, vectorB);
console.log('Cosine Similarity:', distance);
```

## Using SimSIMD in C

For integration within a CMake-based project, add the following segment to your `CMakeLists.txt`:

```cmake
FetchContent_Declare(
    simsimd
    GIT_REPOSITORY https://github.com/ashvardanian/simsimd.git
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(simsimd)
```

If you aim to utilize the `_Float16` functionality with SimSIMD, ensure your development environment is compatible with C 11.
For other SimSIMD functionalities, C 99 compatibility will suffice.
A minimal usage example would be:

```c
#include <simsimd/simsimd.h>

int main() {
    simsimd_f32_t vector_a[1536];
    simsimd_f32_t vector_b[1536];
    simsimd_f32_t distance = simsimd_cos_f32_skylake(vector_a, vector_b, 1536);
    return 0;
}
```

All of the function names follow the same pattern: `simsimd_{metric}_{type}_{backend}`.

- The backend can be `avx512`, `avx2`, `neon`, or `sve`.
- The type can be `f64`, `f32`, `f16`, `i8`, or `b8`.
- The metric can be `cos`, `ip`, `l2sq`, `hamming`, `jaccard`, `kl`, or `js`.

To avoid hard-coding the backend, you can use the `simsimd_metric_punned_t` to pun the function pointer and the `simsimd_capabilities` function to get the available backends at runtime.
Moreover, you can enable `SIMSIMD_DYNAMIC_DISPATCH` and use the precompiled sahred library to avoid recompiling the code for different backends.
When `simsimd_dot_f32`, or one of the following functions, is called, the library will scan available micro-kernels and pick the most advanced one for the current CPU.

```c
// Inner products
void simsimd_dot_f16(simsimd_f16_t const *, simsimd_f16_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_dot_f32(simsimd_f32_t const *, simsimd_f32_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_dot_f64(simsimd_f64_t const *, simsimd_f64_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_dot_f16c(simsimd_f16_t const *, simsimd_f16_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_dot_f32c(simsimd_f32_t const *, simsimd_f32_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_dot_f64c(simsimd_f64_t const *, simsimd_f64_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_vdot_f16c(simsimd_f16_t const *, simsimd_f16_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_vdot_f32c(simsimd_f32_t const *, simsimd_f32_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_vdot_f64c(simsimd_f64_t const *, simsimd_f64_t const *, simsimd_size_t, simsimd_distance_t *);

// Spatial distances
void simsimd_cos_i8(simsimd_i8_t const *, simsimd_i8_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_cos_f16(simsimd_f16_t const *, simsimd_f16_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_cos_f32(simsimd_f32_t const *, simsimd_f32_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_cos_f64(simsimd_f64_t const *, simsimd_f64_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_l2sq_i8(simsimd_i8_t const *, simsimd_i8_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_l2sq_f16(simsimd_f16_t const *, simsimd_f16_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_l2sq_f32(simsimd_f32_t const *, simsimd_f32_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_l2sq_f64(simsimd_f64_t const *, simsimd_f64_t const *, simsimd_size_t, simsimd_distance_t *);

// Binary distances
void simsimd_hamming_b8(simsimd_b8_t const *, simsimd_b8_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_jaccard_b8(simsimd_b8_t const *, simsimd_b8_t const *, simsimd_size_t, simsimd_distance_t *);

// Probability distributions
void simsimd_kl_f16(simsimd_f16_t const *, simsimd_f16_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_kl_f32(simsimd_f32_t const *, simsimd_f32_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_kl_f64(simsimd_f64_t const *, simsimd_f64_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_js_f16(simsimd_f16_t const *, simsimd_f16_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_js_f32(simsimd_f32_t const *, simsimd_f32_t const *, simsimd_size_t, simsimd_distance_t *);
void simsimd_js_f64(simsimd_f64_t const *, simsimd_f64_t const *, simsimd_size_t, simsimd_distance_t *);
```
