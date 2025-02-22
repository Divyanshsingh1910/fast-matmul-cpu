// salykova.c
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

int main(void) {
    srand((unsigned)time(NULL));
    printf("MatrixSize, GFLOPS\n");

    // Loop over sizes: 32, 64, 128, ... , 8192
    for (int size = 32; size <= 8192; size *= 2) {
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
    }
    return 0;
}

