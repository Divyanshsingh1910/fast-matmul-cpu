//gcc -O3 -march=native -I${MKLROOT}/include bench.c -o benchmark.out\
-fopenmp -L${MKLROOT}/lib -lmkl_intel_lp64 -lmkl_core -lmkl_gnu_thread -lpthread -lm -ldl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mkl.h> // Intel MKL header
#include <immintrin.h> // For _mm_malloc
#include <stdint.h> // For uint64_t

#define MEMALIGN 64
#define BATCH_SIZE 1  // Batch size for BRGEMM

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

// Function to run SGEMM benchmark
double benchmark_sgemm(int m, int n, int k, int iterations) {

    // Allocate matrices with proper alignment
    float *A = (float*)mkl_malloc(m * k * sizeof(float), MEMALIGN);
    float *B = (float*)mkl_malloc(k * n * sizeof(float), MEMALIGN);
    float *C = (float*)mkl_malloc(m * n * sizeof(float), MEMALIGN);
    
    if (A == NULL || B == NULL || C == NULL) {
        fprintf(stderr, "SGEMM: Memory allocation failed\n");
        return 1;
    }
    
    // Initialize matrices with random values
    fill_random(A, m, k);
    fill_random(B, k, n);


    double total_time = 0.0;
    
    for (int iter = 0; iter < iterations; iter++) {
        // Zero out C for each run
        for (int i = 0; i < m * n; i++) {
            C[i] = 0.0f;
        }
        
        uint64_t start = get_time_ns();
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                   m, n, k, 1.0f, A, k, B, n, 0.0f, C, n);
        uint64_t end = get_time_ns();
        
        total_time += (end - start) / 1e9;
    }

    // Clean up
    mkl_free(A);
    mkl_free(B);
    mkl_free(C);    
    
    return total_time / iterations;
}

// Function to run BRGEMM benchmark using cblas_sgemm_batch
double benchmark_brgemm(int m, int n, int k, int iterations) {
    // Allocate matrices with proper alignment
    float *A = (float*)mkl_malloc(m * k * sizeof(float), MEMALIGN);
    float *B = (float*)mkl_malloc(k * n * sizeof(float), MEMALIGN);
    float *C = (float*)mkl_malloc(m * n * sizeof(float), MEMALIGN);

    double total_time = 0.0;
    
    // Create arrays for batch processing
    float **A_array = (float**)malloc(BATCH_SIZE * sizeof(float*));
    float **B_array = (float**)malloc(BATCH_SIZE * sizeof(float*));
    float **C_array = (float**)malloc(BATCH_SIZE * sizeof(float*));
    
    // Set up BRGEMM parameters
    MKL_INT lda = k;
    MKL_INT ldb = n;
    MKL_INT ldc = n;
    
    // Since we're doing a single matmul with BRGEMM (BATCH_SIZE=1),
    // we can directly use the input matrices
    A_array[0] = A;
    B_array[0] = B;
    
    // Constants for cblas_sgemm_batch
    const float alpha = 1.0f;
    const float beta = 0.0f;
    const MKL_INT group_count = 1;
    const MKL_INT group_size = BATCH_SIZE;
    
    // Create arrays for matrix dimensions, all matrices are the same size
    MKL_INT m_array[1] = {m};
    MKL_INT n_array[1] = {n};
    MKL_INT k_array[1] = {k};
    MKL_INT lda_array[1] = {lda};
    MKL_INT ldb_array[1] = {ldb};
    MKL_INT ldc_array[1] = {ldc};
    float alpha_array[1] = {alpha};
    float beta_array[1] = {beta};
    
    // Transpose parameters
    CBLAS_TRANSPOSE transa_array[1] = {CblasNoTrans};
    CBLAS_TRANSPOSE transb_array[1] = {CblasNoTrans};
    
    for (int iter = 0; iter < iterations; iter++) {
        // Zero out C for each run
        memset(C, 0, m * n * sizeof(float));
        C_array[0] = C;
        
        uint64_t start = get_time_ns();
        
        // Call cblas_sgemm_batch for batch processing
        cblas_sgemm_batch(
            CblasRowMajor,
            transa_array, transb_array,
            m_array, n_array, k_array,
            alpha_array,
            (const float**)A_array, lda_array,
            (const float**)B_array, ldb_array,
            beta_array,
            C_array, ldc_array,
            group_count, &group_size
        );
        
        uint64_t end = get_time_ns();
        total_time += (end - start) / 1e9;
    }
    
    // Clean up
    free(A_array);
    free(B_array);
    free(C_array);

    // Clean up
    mkl_free(A);
    mkl_free(B);
    mkl_free(C);    
    
    return total_time / iterations;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <m> <n> <k> <n_iter>\n", argv[0]);
        return 1;
    }
    
    int m = atoi(argv[1]), n = atoi(argv[2]), k = atoi(argv[3]);
    if (m <= 0 || n <= 0 || k <= 0) {
        fprintf(stderr, "Invalid matrix size: %d, %d, %d\n", m, n, k);
        return 1;
    }
    
    srand((unsigned)time(NULL));
    printf("MatrixSize, SGEMM_GFLOPS, BRGEMM_GFLOPS\n");

    int iterations = atoi(argv[4]);
    double flops = 2.0 * m * n * k; // 2*m*n*k floating point ops
    
    // Benchmark SGEMM
    double sgemm_time = benchmark_sgemm(m, n, k, iterations);
    double sgemm_gflops = (flops / sgemm_time) / 1e9;
    
    // Benchmark BRGEMM
    double brgemm_time = benchmark_brgemm(m, n, k, iterations);
    double brgemm_gflops = (flops / brgemm_time) / 1e9;
    
    // Print results
    printf("[%d,%d,%d], %f, %f\n", m, n, k, sgemm_gflops, brgemm_gflops);
    
    
    return 0;
}