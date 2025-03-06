//gcc -O3 -march=native \
//  -I${MKLROOT}/include mkl_dnn.c -S benchmark_avx2_omp.s -fopenmp \
//  -L${MKLROOT}/lib -lmkl_intel_lp64 -lmkl_core -lmkl_gnu_thread -lpthread -lm -ldl

// export MKL_ENABLE_INSTRUCTIONS=AVX2
// export MKL_ENABLE_INSTRUCTIONS=AVX512
// export MKL_ENABLE_INSTRUCTIONS=AVX512_E4

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mkl.h>       // Intel MKL header
#include <immintrin.h> // For _mm_malloc

#define MEMALIGN 64

// Get current time in nanoseconds
uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Fill a matrix of dimensions m x n with random floats in [-1, 1]
void fill_random(float *mat, int m, int n) {
    for (int i = 0; i < m * n; i++) {
        mat[i] = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }
}

int main(void) {
    srand((unsigned)time(NULL));
    printf("MatrixSize, GFLOPS\n");

    // Loop over sizes: 64, 128, 256, ... , 8192
    for (int size = 32; size <= 8192; size *= 2) {
        int m = size, n = size, k = size;
        // Allocate matrices with proper alignment
        float *A = (float*)_mm_malloc(m * k * sizeof(float), MEMALIGN);
        float *B = (float*)_mm_malloc(k * n * sizeof(float), MEMALIGN);
        float *C = (float*)_mm_malloc(m * n * sizeof(float), MEMALIGN);

        fill_random(A, m, k);
        fill_random(B, k, n);

        // Warm-up call
#ifdef COL_MAJOR
        //printf("col-major chal rha bhai\n");
        cblas_sgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
            m, n, k, 1.0f, A, m, B, k, 0.0f, C, m);
#else
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    m, n, k, 1.0f, A, k, B, n, 0.0f, C, n);
#endif
        int iterations = 10;
        double total_time = 0.0;
        double flops = 2.0 * m * n * k; // 2*m*n*k floating point ops
        _mm_free(A);
        _mm_free(B);
        _mm_free(C);

        for (int iter = 0; iter < iterations; iter++) {
            float *A = (float*)_mm_malloc(m * k * sizeof(float), MEMALIGN);
            float *B = (float*)_mm_malloc(k * n * sizeof(float), MEMALIGN);
            float *C = (float*)_mm_malloc(m * n * sizeof(float), MEMALIGN);
            // Zero out C for each run
            for (int i = 0; i < m * n; i++) {
                C[i] = 0.0f;
            }
            uint64_t start = get_time_ns();
#ifdef COL_MAJOR
        cblas_sgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
            m, n, k, 1.0f, A, m, B, k, 0.0f, C, m);
#else
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    m, n, k, 1.0f, A, k, B, n, 0.0f, C, n);
#endif
            uint64_t end = get_time_ns();
            total_time += (end - start) / 1e9;
            _mm_free(A);
            _mm_free(B);
            _mm_free(C);
        }
        double avg_time = total_time / iterations;
        double gflops = (flops / avg_time) / 1e9;
        printf("%d, %f\n", size, gflops);

    }
    return 0;
}

