// salykova.c
// gcc -O3 -march=native -fopenmp bench.c matmul.c kernel.c -o salykova -DNTHREADS=72
// -DMR=16 -DNR=6 -DMC_factor=2 -NC_factor=71

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <immintrin.h>
#include "matmul.h"  // Use the matmul library from CMakeLists.txt

#define MEMALIGN 64

// Get current time in nanoseconds
uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Fill a matrix of dimensions rows x cols with random floats in [-1, 1]
void fill_random(float *mat, int rows, int cols) {
    for (int i = 0; i < rows * cols; i++) {
        mat[i] = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    }
}

int main(int argc, char *argv[]) {
    srand((unsigned)time(NULL));
    printf("MatrixSize, GFLOPS\n");

    // Loop over sizes: 32, 64, 128, ... , 8192
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <matrix_size>\n", argv[0]);
        return 1;
    }
    int size = atoi(argv[1]);
    if (size <= 0) {
        fprintf(stderr, "Invalid matrix size: %d\n", size);
        return 1;
    }

    int m = size, n = size, k = size;
    // Allocate matrices with proper alignment (column-major layout)
    float *A = (float*)_mm_malloc(m * k * sizeof(float), MEMALIGN);
    float *B = (float*)_mm_malloc(k * n * sizeof(float), MEMALIGN);
    float *C = (float*)_mm_malloc(m * n * sizeof(float), MEMALIGN);

    fill_random(A, m, k);
    fill_random(B, k, n);

    // Warm-up call
    matmul(A, B, C, m, n, k);

    int iterations = 10;
    double total_time = 0.0;
    double flops = 2.0 * m * n * k; // 2*m*n*k floating point ops

    for (int iter = 0; iter < iterations; iter++) {
        // Zero out C for each run
        for (int i = 0; i < m * n; i++) {
            C[i] = 0.0f;
        }
        uint64_t start = get_time_ns();
        matmul(A, B, C, m, n, k);
        uint64_t end = get_time_ns();
        total_time += (end - start) / 1e9;
    }
    double avg_time = total_time / iterations;
    double gflops = (flops / avg_time) / 1e9;
    printf("%d, %f\n", size, gflops);

    _mm_free(A);
    _mm_free(B);
    _mm_free(C);
    
    return 0;
}

