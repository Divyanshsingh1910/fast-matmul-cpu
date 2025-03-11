//gcc -O3 -march=native -I${MKLROOT}/include bench.c -o benchmark_batch.out \
-fopenmp -L${MKLROOT}/lib -lmkl_intel_lp64 -lmkl_core -lmkl_gnu_thread -lpthread -lm -ldl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mkl.h> // Intel MKL header
#include <immintrin.h> // For _mm_malloc
#include <stdint.h> // For uint64_t
#include <omp.h>    // For OpenMP

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

// Function to run batched SGEMM benchmark with OpenMP parallelization
double benchmark_batched_sgemm(int m, int n, int k, int batch_size, int iterations) {
    // Allocate arrays of matrices
    float **A_array = (float**)malloc(batch_size * sizeof(float*));
    float **B_array = (float**)malloc(batch_size * sizeof(float*));
    float **C_array = (float**)malloc(batch_size * sizeof(float*));
    
    if (A_array == NULL || B_array == NULL || C_array == NULL) {
        fprintf(stderr, "Memory allocation for arrays failed\n");
        return -1.0;
    }
    
    // Allocate and initialize individual matrices
    for (int i = 0; i < batch_size; i++) {
        A_array[i] = (float*)mkl_malloc(m * k * sizeof(float), MEMALIGN);
        B_array[i] = (float*)mkl_malloc(k * n * sizeof(float), MEMALIGN);
        C_array[i] = (float*)mkl_malloc(m * n * sizeof(float), MEMALIGN);
        
        if (A_array[i] == NULL || B_array[i] == NULL || C_array[i] == NULL) {
            fprintf(stderr, "Memory allocation for matrices failed\n");
            return -1.0;
        }
        
        // Initialize with random values
        fill_random(A_array[i], m, k);
        fill_random(B_array[i], k, n);
    }
    
    double total_time = 0.0;
    
    for (int iter = 0; iter < iterations; iter++) {
        // Zero out all C matrices for each run
        for (int i = 0; i < batch_size; i++) {
            memset(C_array[i], 0, m * n * sizeof(float));
        }
        
        uint64_t start = get_time_ns();
        
        // Run SGEMM in parallel for each matrix in the batch
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < batch_size; i++) {
            cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                       m, n, k, 1.0f, A_array[i], k, B_array[i], n, 0.0f, C_array[i], n);
        }
        
        uint64_t end = get_time_ns();
        total_time += (end - start) / 1e9;
    }
    
    // Clean up
    for (int i = 0; i < batch_size; i++) {
        mkl_free(A_array[i]);
        mkl_free(B_array[i]);
        mkl_free(C_array[i]);
    }
    
    free(A_array);
    free(B_array);
    free(C_array);
    
    return total_time / iterations;
}

// Function to run BRGEMM benchmark using cblas_sgemm_batch
double benchmark_brgemm_batch(int m, int n, int k, int batch_size, int iterations) {
    // Allocate arrays of matrices
    float **A_array = (float**)malloc(batch_size * sizeof(float*));
    float **B_array = (float**)malloc(batch_size * sizeof(float*));
    float **C_array = (float**)malloc(batch_size * sizeof(float*));
    
    if (A_array == NULL || B_array == NULL || C_array == NULL) {
        fprintf(stderr, "Memory allocation for arrays failed\n");
        return -1.0;
    }
    
    // Allocate and initialize individual matrices
    for (int i = 0; i < batch_size; i++) {
        A_array[i] = (float*)mkl_malloc(m * k * sizeof(float), MEMALIGN);
        B_array[i] = (float*)mkl_malloc(k * n * sizeof(float), MEMALIGN);
        C_array[i] = (float*)mkl_malloc(m * n * sizeof(float), MEMALIGN);
        
        if (A_array[i] == NULL || B_array[i] == NULL || C_array[i] == NULL) {
            fprintf(stderr, "Memory allocation for matrices failed\n");
            return -1.0;
        }
        
        // Initialize with random values
        fill_random(A_array[i], m, k);
        fill_random(B_array[i], k, n);
    }
    
    // Set up BRGEMM parameters
    MKL_INT lda = k;
    MKL_INT ldb = n;
    MKL_INT ldc = n;
    
    // Constants for cblas_sgemm_batch
    const float alpha = 1.0f;
    const float beta = 0.0f;
    const MKL_INT group_count = 1;
    const MKL_INT group_size = batch_size;
    
    // Create arrays for matrix dimensions, all matrices are the same size
    MKL_INT *m_array = (MKL_INT*)malloc(sizeof(MKL_INT));
    MKL_INT *n_array = (MKL_INT*)malloc(sizeof(MKL_INT));
    MKL_INT *k_array = (MKL_INT*)malloc(sizeof(MKL_INT));
    MKL_INT *lda_array = (MKL_INT*)malloc(sizeof(MKL_INT));
    MKL_INT *ldb_array = (MKL_INT*)malloc(sizeof(MKL_INT));
    MKL_INT *ldc_array = (MKL_INT*)malloc(sizeof(MKL_INT));
    float *alpha_array = (float*)malloc(sizeof(float));
    float *beta_array = (float*)malloc(sizeof(float));
    
    m_array[0] = m;
    n_array[0] = n;
    k_array[0] = k;
    lda_array[0] = lda;
    ldb_array[0] = ldb;
    ldc_array[0] = ldc;
    alpha_array[0] = alpha;
    beta_array[0] = beta;
    
    // Transpose parameters
    CBLAS_TRANSPOSE *transa_array = (CBLAS_TRANSPOSE*)malloc(sizeof(CBLAS_TRANSPOSE));
    CBLAS_TRANSPOSE *transb_array = (CBLAS_TRANSPOSE*)malloc(sizeof(CBLAS_TRANSPOSE));
    transa_array[0] = CblasNoTrans;
    transb_array[0] = CblasNoTrans;
    
    double total_time = 0.0;
    
    for (int iter = 0; iter < iterations; iter++) {
        // Zero out all C matrices for each run
        for (int i = 0; i < batch_size; i++) {
            memset(C_array[i], 0, m * n * sizeof(float));
        }
        
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
    for (int i = 0; i < batch_size; i++) {
        mkl_free(A_array[i]);
        mkl_free(B_array[i]);
        mkl_free(C_array[i]);
    }
    
    free(A_array);
    free(B_array);
    free(C_array);
    free(m_array);
    free(n_array);
    free(k_array);
    free(lda_array);
    free(ldb_array);
    free(ldc_array);
    free(alpha_array);
    free(beta_array);
    free(transa_array);
    free(transb_array);
    
    return total_time / iterations;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <m> <n> <k> <batch_size> <n_iter>\n", argv[0]);
        return 1;
    }
    
    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);
    int batch_size = atoi(argv[4]);
    int iterations = atoi(argv[5]);
    
    if (m <= 0 || n <= 0 || k <= 0 || batch_size <= 0 || iterations <= 0) {
        fprintf(stderr, "Invalid parameters: m=%d, n=%d, k=%d, batch_size=%d, iterations=%d\n", 
                m, n, k, batch_size, iterations);
        return 1;
    }
    
    // Set random seed
    srand((unsigned)time(NULL));
    
    // Print header
    printf("Matrix Size, Batch Size, SGEMM_GFLOPS, BRGEMM_GFLOPS, Speedup\n");
    
    // Calculate total FLOPS for the batch
    // Each GEMM operation requires 2*m*n*k floating point operations
    double total_flops = 2.0 * m * n * k * batch_size;
    
    // Benchmark batched SGEMM with OpenMP
    double sgemm_time = benchmark_batched_sgemm(m, n, k, batch_size, iterations);
    double sgemm_gflops = (total_flops / sgemm_time) / 1e9;
    
    // Benchmark BRGEMM
    double brgemm_time = benchmark_brgemm_batch(m, n, k, batch_size, iterations);
    double brgemm_gflops = (total_flops / brgemm_time) / 1e9;
    
    // Calculate speedup
    double speedup = sgemm_time / brgemm_time;
    
    // Print results
    printf("[%d,%d,%d], %d, %f, %f, %f\n", 
           m, n, k, batch_size, sgemm_gflops, brgemm_gflops, speedup);
    
    return 0;
}