// -*- mode:c++;indent-tabs-mode:nil;c-basic-offset:4;coding:utf-8 -*-
// vi: set et ft=c++ ts=4 sts=4 sw=4 fenc=utf-8 :vi
//
// Copyright 2024 Mozilla Foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*

g++ -g -Wall -O3 -fopenmp -pthread -DMKL_ILP64 \
  -I/opt/intel/oneapi/mkl/2024.1/include -Wframe-larger-than=65536 \
  -Walloca-larger-than=65536 -march=native -c -o asd.o asd.cpp
g++ -g -Wall -O3 -fopenmp -pthread -DMKL_ILP64 \
  -I/opt/intel/oneapi/mkl/2024.1/include -Wframe-larger-than=65536 \
  -Walloca-larger-than=65536 -o asd asd.o -Wl,--start-group \
  /opt/intel/oneapi/mkl/2024.1/lib/libmkl_intel_ilp64.a \
  /opt/intel/oneapi/mkl/2024.1/lib/libmkl_gnu_thread.a \
  /opt/intel/oneapi/mkl/2024.1/lib/libmkl_core.a -Wl,--end-group -lm
./asd

*/

//
//                   _   _          ___ _      _   ___
//                  | |_(_)_ _ _  _| _ ) |    /_\ / __|
//                  |  _| | ' \ || | _ \ |__ / _ \\__ \.
//                   \__|_|_||_\_, |___/____/_/ \_\___/
//                             |__/
//
//                    BASIC LINEAR ALGEBRA SUBPROGRAMS
//
//
// This file implements multithreaded CPU matrix multiplication for the
// common contiguous use case C = Aᵀ * B. These kernels are designed to
// have excellent performance[1] for matrices that fit in the CPU cache
// without imposing any overhead such as cache filling or malloc calls.
//
// This implementation does not guarantee any upper bound with rounding
// errors, which grow along with k. Our goal's to maximally exploit the
// hardware for performance, and then use whatever resources remain for
// improving numerical accuracy.
//
// [1] J. Tunney, ‘LLaMA Now Goes Faster on CPUs’, Mar. 2024. [Online].
//     Available: https://justine.lol/matmul/. [Accessed: 29-Mar-2024].

#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wnarrowing"
#pragma GCC diagnostic ignored "-Wignored-attributes"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <errno.h>
#include <thread>
#include <unistd.h>
#include <omp.h>
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#ifdef __x86_64__
#include <immintrin.h>
#endif
#include <mkl_blas.h>
//#include <mkl.h>

#ifdef _OPENMP
#define HAVE_OPENMP 1
#else
#define HAVE_OPENMP 0
#endif

#if defined(__ARM_NEON) || defined(__AVX512F__)
#define VECTOR_REGISTERS 32
#else
#define VECTOR_REGISTERS 16
#endif

namespace {

inline int rand32(void) {
    static unsigned long long lcg = 1;
    lcg *= 6364136223846793005;
    lcg += 1442695040888963407;
    return lcg >> 32;
}

inline float float01(unsigned x) { // (0,1)
    return 1.f / 8388608 * ((x >> 9) + .5f);
}

inline float numba(void) { // (-1,1)
    return float01(rand32()) * 2 - 1;
}

template <typename T> void clean(int m, int n, T *A, int lda) {
    for (int i = 0; i < m; ++i)
        for (int j = n; j < lda; ++j)
            A[lda * i + j] = 0;
}

template <typename T> void randomize(int m, int n, T *A, int lda) {
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j)
            A[lda * i + j] = numba();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// VECTORIZED ARITHMETIC OPERATIONS

inline float add(float x, float y) {
    return x + y;
}
inline float sub(float x, float y) {
    return x - y;
}
inline float mul(float x, float y) {
    return x * y;
}

#if defined(__SSE__) || defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
inline __m128 add(__m128 x, __m128 y) {
    return _mm_add_ps(x, y);
}
inline __m128 sub(__m128 x, __m128 y) {
    return _mm_sub_ps(x, y);
}
inline __m128 mul(__m128 x, __m128 y) {
    return _mm_mul_ps(x, y);
}
#endif // __SSE__

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
inline __m256 add(__m256 x, __m256 y) {
    return _mm256_add_ps(x, y);
}
inline __m256 sub(__m256 x, __m256 y) {
    return _mm256_sub_ps(x, y);
}
inline __m256 mul(__m256 x, __m256 y) {
    return _mm256_mul_ps(x, y);
}
#endif // __AVX__

#if defined(__AVX512F__)
inline __m512 add(__m512 x, __m512 y) {
    return _mm512_add_ps(x, y);
}
inline __m512 sub(__m512 x, __m512 y) {
    return _mm512_sub_ps(x, y);
}
inline __m512 mul(__m512 x, __m512 y) {
    return _mm512_mul_ps(x, y);
}
#endif // __AVX512F__

#if defined(__ARM_NEON)
inline float32x4_t add(float32x4_t x, float32x4_t y) {
    return vaddq_f32(x, y);
}
inline float32x4_t sub(float32x4_t x, float32x4_t y) {
    return vsubq_f32(x, y);
}
inline float32x4_t mul(float32x4_t x, float32x4_t y) {
    return vmulq_f32(x, y);
}
#endif // __ARM_NEON

////////////////////////////////////////////////////////////////////////////////////////////////////
// VECTORIZED HORIZONTAL SUM

inline float hsum(float x) {
    return x;
}

#if defined(__ARM_NEON)
inline float hsum(float32x4_t x) {
    return vaddvq_f32(x);
}
#endif // __ARM_NEON

#if defined(__SSE__) || defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
inline float hsum(__m128 x) {
#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
    x = _mm_add_ps(x, _mm_movehl_ps(x, x));
    x = _mm_add_ss(x, _mm_movehdup_ps(x));
#else
    __m128 t;
    t = _mm_shuffle_ps(x, x, _MM_SHUFFLE(2, 3, 0, 1));
    x = _mm_add_ps(x, t);
    t = _mm_movehl_ps(t, x);
    x = _mm_add_ss(x, t);
#endif
    return _mm_cvtss_f32(x);
}
#endif

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
inline float hsum(__m256 x) {
    return hsum(_mm_add_ps(_mm256_extractf128_ps(x, 1), _mm256_castps256_ps128(x)));
}
#endif // __AVX__

#if defined(__AVX512F__)
inline float hsum(__m512 x) {
    return _mm512_reduce_add_ps(x);
}
#endif // __AVX512F__

////////////////////////////////////////////////////////////////////////////////////////////////////
// VECTORIZED MEMORY LOADING

template <typename T, typename U> T load(const U *);

template <> inline float load(const float *p) {
    return *p;
}

#if defined(__ARM_NEON)
template <> inline float32x4_t load(const float *p) {
    return vld1q_f32(p);
}
#endif // __ARM_NEON

#if defined(__SSE__) || defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
template <> inline __m128 load(const float *p) {
    return _mm_loadu_ps(p);
}
#endif // __SSE__

#if defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__)
template <> inline __m256 load(const float *p) {
    return _mm256_loadu_ps(p);
}
#endif // __AVX__

#if defined(__AVX512F__)
template <> inline __m512 load(const float *p) {
    return _mm512_loadu_ps(p);
}
#endif // __AVX512F__

////////////////////////////////////////////////////////////////////////////////////////////////////
// ABSTRACTIONS

/**
 * Computes a * b + c.
 *
 * This operation will become fused into a single arithmetic instruction
 * if the hardware has support for this feature, e.g. Intel Haswell+ (c.
 * 2013), AMD Bulldozer+ (c. 2011), etc.
 */
template <typename T, typename U> inline U madd(T a, T b, U c) {
    return add(mul(a, b), c);
}

/**
 * Computes a * b + c with error correction.
 *
 * @see W. Kahan, "Further remarks on reducing truncation errors,"
 *    Communications of the ACM, vol. 8, no. 1, p. 40, Jan. 1965,
 *    doi: 10.1145/363707.363723.
 */
template <typename T, typename U> inline U madder(T a, T b, U c, U *e) {
    U y = sub(mul(a, b), *e);
    U t = add(c, y);
    *e = sub(sub(t, c), y);
    return t;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FLOATING POINT MATRIX MULTIPLICATION

template <int KN, typename DOT, typename VECTOR, typename TA, typename TB, typename TC>
class tinyBLAS {
  public:
    tinyBLAS(const TA *A, int lda, const TB *B, int ldb, TC *C, int ldc, int ith, int nth)
        : A(A), B(B), C(C), lda(lda), ldb(ldb), ldc(ldc), ith(ith), nth(nth) {
    }

    void matmul(int m, int n, int k) {
        mnpack(0, m, 0, n, k);
    }

  private:
    void mnpack(int m0, int m, int n0, int n, int k) {
        int mc, nc, mp, np;
        switch ((std::min(m - m0, 5) << 4) | std::min(n - n0, 5)) {
#if VECTOR_REGISTERS == 32
        case 0x55:
            mc = 5;
            nc = 5;
            gemm<5, 5>(m0, m, n0, n, k);
            break;
        case 0x45:
            mc = 4;
            nc = 5;
            gemm<4, 5>(m0, m, n0, n, k);
            break;
        case 0x54:
            mc = 5;
            nc = 4;
            gemm<5, 4>(m0, m, n0, n, k);
            break;
        case 0x44:
            mc = 4;
            nc = 4;
            gemm<4, 4>(m0, m, n0, n, k);
            break;
        case 0x53:
            mc = 5;
            nc = 3;
            gemm<5, 3>(m0, m, n0, n, k);
            break;
        case 0x35:
            mc = 3;
            nc = 5;
            gemm<3, 5>(m0, m, n0, n, k);
            break;
        case 0x43:
            mc = 4;
            nc = 3;
            gemm<4, 3>(m0, m, n0, n, k);
            break;
#else
        case 0x55:
        case 0x54:
        case 0x53:
        case 0x45:
        case 0x44:
        case 0x43:
            mc = 4;
            nc = 3;
            gemm<4, 3>(m0, m, n0, n, k);
            break;
        case 0x35:
#endif
        case 0x34:
            mc = 3;
            nc = 4;
            gemm<3, 4>(m0, m, n0, n, k);
            break;
        case 0x52:
            mc = 5;
            nc = 2;
            gemm<5, 2>(m0, m, n0, n, k);
            break;
        case 0x33:
            mc = 3;
            nc = 3;
            gemm<3, 3>(m0, m, n0, n, k);
            break;
        case 0x25:
            mc = 2;
            nc = 5;
            gemm<2, 5>(m0, m, n0, n, k);
            break;
        case 0x42:
            mc = 4;
            nc = 2;
            gemm<4, 2>(m0, m, n0, n, k);
            break;
        case 0x24:
            mc = 2;
            nc = 4;
            gemm<2, 4>(m0, m, n0, n, k);
            break;
        case 0x32:
            mc = 3;
            nc = 2;
            gemm<3, 2>(m0, m, n0, n, k);
            break;
        case 0x23:
            mc = 2;
            nc = 3;
            gemm<2, 3>(m0, m, n0, n, k);
            break;
        case 0x51:
            mc = 5;
            nc = 1;
            gemm<5, 1>(m0, m, n0, n, k);
            break;
        case 0x41:
            mc = 4;
            nc = 1;
            gemm<4, 1>(m0, m, n0, n, k);
            break;
        case 0x22:
            mc = 2;
            nc = 2;
            gemm<2, 2>(m0, m, n0, n, k);
            break;
        case 0x15:
            mc = 1;
            nc = 5;
            gemm<1, 5>(m0, m, n0, n, k);
            break;
        case 0x14:
            mc = 1;
            nc = 4;
            gemm<1, 4>(m0, m, n0, n, k);
            break;
        case 0x31:
            mc = 3;
            nc = 1;
            gemm<3, 1>(m0, m, n0, n, k);
            break;
        case 0x13:
            mc = 1;
            nc = 3;
            gemm<1, 3>(m0, m, n0, n, k);
            break;
        case 0x21:
            mc = 2;
            nc = 1;
            gemm<2, 1>(m0, m, n0, n, k);
            break;
        case 0x12:
            mc = 1;
            nc = 2;
            gemm<1, 2>(m0, m, n0, n, k);
            break;
        case 0x11:
            mc = 1;
            nc = 1;
            gemm<1, 1>(m0, m, n0, n, k);
            break;
        default:
            return;
        }
        mp = m0 + (m - m0) / mc * mc;
        np = n0 + (n - n0) / nc * nc;
        mnpack(mp, m, n0, np, k);
        mnpack(m0, m, np, n, k);
    }

    template <int RM, int RN> void gemm(int m0, int m, int n0, int n, int k) {
        int ytiles = (m - m0) / RM;
        int xtiles = (n - n0) / RN;
        int tiles = xtiles * ytiles;
        int duty = (tiles + nth - 1) / nth;
        int start = duty * ith;
        int end = start + duty;
        if (end > tiles)
            end = tiles;
        for (int job = start; job < end; ++job) {
            int ii = m0 + job / xtiles * RM;
            int jj = n0 + job % xtiles * RN;
            DOT Cv[RN][RM] = {0};
            for (int l = 0; l < k; l += KN)
                for (int j = 0; j < RN; ++j)
                    for (int i = 0; i < RM; ++i)
                        Cv[j][i] = madd(load<VECTOR>(A + lda * (ii + i) + l), //
                                        load<VECTOR>(B + ldb * (jj + j) + l), //
                                        Cv[j][i]);
            TC Cd[RN][RM];
            for (int j = 0; j < RN; ++j)
                for (int i = 0; i < RM; ++i)
                    Cd[j][i] = hsum(Cv[j][i]);
            for (int j = 0; j < RN; ++j)
                for (int i = 0; i < RM; ++i)
                    C[ldc * (jj + j) + (ii + i)] = Cd[j][i];
        }
    }

    const TA *const A;
    const TB *const B;
    TC *const C;
    const int lda;
    const int ldb;
    const int ldc;
    const int ith;
    const int nth;
};

static void llamafile_sgemm_impl(int m, int n, int k, const float *A, int lda, const float *B,
                                 int ldb, float *C, int ldc, int ith, int nth) {
    assert(m >= 0);
    assert(n >= 0);
    assert(k >= 0);
    assert(lda >= k);
    assert(ldb >= k);
    assert(ldc >= m);
    assert(nth > 0);
    assert(ith < nth);
    assert(1ll * lda * m <= 0x7fffffff);
    assert(1ll * ldb * n <= 0x7fffffff);
    assert(1ll * ldc * n <= 0x7fffffff);
#if defined(__AVX512F__)
    assert(!(lda % (64 / sizeof(float))));
    assert(!(ldb % (64 / sizeof(float))));
    tinyBLAS<16, __m512, __m512, float, float, float> tb{A, lda, B, ldb, C, ldc, ith, nth};
    tb.matmul(m, n, k);
#elif defined(__AVX__) || defined(__AVX2__)
    assert(!(lda % (32 / sizeof(float))));
    assert(!(ldb % (32 / sizeof(float))));
    tinyBLAS<8, __m256, __m256, float, float, float> tb{A, lda, B, ldb, C, ldc, ith, nth};
    tb.matmul(m, n, k);
#elif defined(__SSE__)
    assert(!(lda % (16 / sizeof(float))));
    assert(!(ldb % (16 / sizeof(float))));
    tinyBLAS<4, __m128, __m128, float, float, float> tb{A, lda, B, ldb, C, ldc, ith, nth};
    tb.matmul(m, n, k);
#elif defined(__ARM_NEON)
    assert(!(lda % (16 / sizeof(float))));
    assert(!(ldb % (16 / sizeof(float))));
    tinyBLAS<4, float32x4_t, float32x4_t, float, float, float> tb{A, lda, B, ldb, C, ldc, ith, nth};
    tb.matmul(m, n, k);
#else
    tinyBLAS<1, float, float, float, float, float> tb{A, lda, B, ldb, C, ldc, ith, nth};
    tb.matmul(m, n, k);
#endif
}

} // namespace

template <typename T, typename U> T *llamafile_malloc(int m, int n, U *out_lda) {
    void *ptr;
    int b = 64 / sizeof(T);
    int lda = (n + b - 1) & -b;
    size_t size = sizeof(T) * m * lda;
    if ((errno = posix_memalign(&ptr, sysconf(_SC_PAGESIZE), size))) {
        perror("posix_memalign");
        exit(1);
    }
    *out_lda = lda;
    return (T *)ptr;
}

template <typename T, typename U> T *llamafile_new_test_matrix(int m, int n, U *out_lda) {
    T *A = llamafile_malloc<T>(m, n, out_lda);
    randomize(m, n, A, *out_lda);
    clean(m, n, A, *out_lda);
    return A;
}

void llamafile_sgemm(int m, int n, int k, const float *A, int lda, const float *B, int ldb,
                     float *C, int ldc, int ith, int nth) {
        printf("#threads:= %d\n",nth);
    if (nth) {
        llamafile_sgemm_impl(m, n, k, A, lda, B, ldb, C, ldc, ith, nth);
        printf("if section\n");
    } else if (!HAVE_OPENMP || 1ll * n * m * k < 3000000) {
        printf("else if section\n");
        llamafile_sgemm_impl(m, n, k, A, lda, B, ldb, C, ldc, 0, 1);
    } else {
        nth = sysconf(_SC_NPROCESSORS_ONLN);
        printf("n=%d\tif section\n",nth);
#pragma omp parallel for
        for (ith = 0; ith < nth; ++ith)
            llamafile_sgemm_impl(m, n, k, A, lda, B, ldb, C, ldc, ith, nth);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////


MKL_INT m = 1024;
MKL_INT n = 1024;
MKL_INT k = 1024;

// MKL_INT m = 512;
// MKL_INT n = 4609;
// MKL_INT k = 784;

// MKL_INT m = 2048;
// MKL_INT n = 2048;
// MKL_INT k = 2048;

float *A, *B, *C;
MKL_INT lda, ldb, ldc;

void multiply_mkl() {
    float beta = 0;
    float alpha = 1;
    SGEMM("T", "N", &m, &n, &k, &alpha, A, &lda, B, &ldb, &beta, C, &ldc);
    volatile float x = C[0];
    (void)x;
}

void multiply_llamafile() {
    llamafile_sgemm(m, n, k, A, lda, B, ldb, C, ldc, 0, 48);
    volatile float x = C[0];
    (void)x;
}

long long micros(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000 + (ts.tv_nsec + 999) / 1000;
}

#define ITERATIONS 1
#define BENCH(x) \
    do { \
        x; \
        long long t1 = micros(); \
        for (long long i = 0; i < ITERATIONS; ++i) { \
            asm volatile("" ::: "memory"); \
            x; \
            asm volatile("" ::: "memory"); \
        } \
        long long t2 = micros(); \
        printf("%8lld µs %s %g gigaflops\n", (t2 - t1 + ITERATIONS - 1) / ITERATIONS, #x, \
               1e6 / ((t2 - t1 + ITERATIONS - 1) / ITERATIONS) * m * n * k * 1e-9); \
    } while (0)

int main(int argc, char* argv[]) {
    if(argc >= 4){
        m = atoi(argv[1]);
        n = atoi(argv[2]);
        k = atoi(argv[3]);
    }
    printf("m=%ld\tn=%ld\tk=%ld\n", m, n, k);
    A = llamafile_new_test_matrix<float>(m, k, &lda);
    B = llamafile_new_test_matrix<float>(n, k, &ldb);
    C = llamafile_new_test_matrix<float>(n, m, &ldc);
    multiply_mkl(); // cold start
    multiply_llamafile(); // cold start
    BENCH(multiply_mkl());
    BENCH(multiply_llamafile());
    BENCH(multiply_mkl());
    BENCH(multiply_llamafile());
}

