/*
 gcc -O3 -march=native -I${MKLROOT}/include brgemm-profile.c -L${MKLROOT}/lib/intel64 -Wl,--no-as-needed -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl -fopenmp -o brgemm_test
*/
#define NDEBUG

#include <assert.h>
#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef LOCAL_MACHINE
    #include<mkl/mkl.h>
#else 
    #include <mkl.h>
#endif /* ifdef LOCAL_MACHINE
 */

#define MEMALIGN 64

// Matrix dimensions
#ifndef DIMB
    #define DIMB 32    // Batch size
#endif
#ifndef DIMT
    #define DIMT 1024   // Sequence length
#endif
#ifndef DIMH
    #define DIMH 4096  // Hidden dimension
#endif

#ifndef NITER
    #define NITER 5
#endif

#define WARMUP_ITER 10

void init_rand(float* A, const int32_t M, const int32_t N) {
    for(int i = 0; i < M*N; i++) {
        A[i] = rand() / (float)RAND_MAX;
    }
}

void init_const(float* A, const float val, const int32_t M, const int32_t N) {
    for(int i = 0; i < M*N; i++) {
        A[i] = val;
    }
}

uint64_t timer() {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    return (uint64_t)start.tv_sec * 1000000000 + (uint64_t)start.tv_nsec;
}

void compare_mats(float* mat1, float* mat2, const int B, const int M, const int N) {
    for (int b = 0; b < B; b++) {
        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                if (fabsf(mat1[b*M*N + i*N + j] - mat2[b*M*N + i*N + j]) > 1e-3) {
                    printf("MISMATCH! Element[%d][%d] %f != %f\n",
                           i, j, mat1[j * M + i], mat2[j * M + i]);
                    return;
                }
            }
        }
    }
    printf("MATCH!\n");
    return;
}

void looped_sgemm(float* A, float* B, float* C, const int32_t B_dim, const int32_t T, const int32_t H) {
    /*
    * A: (B, T, H)
    * B: (B, H, T)
    * C: (B, T, T)
    *
    * Op --> C = alpha * (A @ B) + beta * C
    */
    const float alpha = 1.0f;
    const float beta = 0.0f;
    
    for (int b = 0; b < B_dim; b++) {
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    T, T, H,
                    alpha,
                    A + b * T * H, H,      // stride to next batch in A
                    B + b * H * T, T,      // stride to next batch in B
                    beta,
                    C + b * T * T, T);     // stride to next batch in C
    }
}

void brgemm_kernel(float* A, float* B, float* C, const int32_t B_dim, const int32_t T, const int32_t H) {
    /*
    * A: (B, T, H)
    * B: (B, H, T)
    * C: (B, T, T)
    */
    const float alpha = 1.0f;
    const float beta = 0.0f;
    
    // Create BRGEMM descriptor
    MKL_INT batch_size = B_dim;
    MKL_INT m = T;
    MKL_INT n = T;
    MKL_INT k = H;
    MKL_INT lda = H;
    MKL_INT ldb = T;
    MKL_INT ldc = T;
    
    // Arrays to hold matrix addresses for each batch
    float** A_array = (float**)malloc(batch_size * sizeof(float*));
    float** B_array = (float**)malloc(batch_size * sizeof(float*));
    float** C_array = (float**)malloc(batch_size * sizeof(float*));
    
    // Set up array of addresses
    for (int i = 0; i < batch_size; i++) {
        A_array[i] = A + i * T * H;
        B_array[i] = B + i * H * T;
        C_array[i] = C + i * T * T;
    }
    // Add these lines before the cblas_sgemm_batch call:
    const CBLAS_TRANSPOSE transa = CblasNoTrans;
    const CBLAS_TRANSPOSE transb = CblasNoTrans;
    // Perform batched GEMM
    cblas_sgemm_batch(CblasRowMajor, 
                      &transa, &transb,
                      &m, &n, &k,
                      &alpha,
                      (const float**)A_array, &lda,
                      (const float**)B_array, &ldb,
                      &beta,
                      C_array, &ldc,
                      1, &batch_size);
    
    free(A_array);
    free(B_array);
    free(C_array);
}

void brgemm_kernel_strided(float* A, float* B, float* C, const int32_t B_dim, const int32_t T, const int32_t H) {
    /*
    * A: (B, T, H)
    * B: (B, H, T)
    * C: (B, T, T)
    */
    const float alpha = 1.0f;
    const float beta = 0.0f;

    // BRGEMM parameters
    MKL_INT batch_size = B_dim;
    MKL_INT m = T;
    MKL_INT n = T;
    MKL_INT k = H;
    MKL_INT lda = H;
    MKL_INT ldb = T;
    MKL_INT ldc = T;
    MKL_INT stridea = T * H;
    MKL_INT strideb = H * T;
    MKL_INT stridec = T * T;
    
    const CBLAS_TRANSPOSE transa = CblasNoTrans;
    const CBLAS_TRANSPOSE transb = CblasNoTrans;
    
    // Perform batched GEMM using strided API
    cblas_sgemm_batch_strided(CblasRowMajor, 
                              transa, transb,
                              m, n, k,
                              alpha,
                              A, lda, stridea,
                              B, ldb, strideb,
                              beta,
                              C, ldc, stridec,
                              batch_size);
}

void (*matmul_kernel)(float* A, float* B, float* C, const int32_t B_dim, const int32_t T, const int32_t H);


int main() {
    srand(1);  // For reproducible results
    
    const int32_t B_dim = DIMB;
    const int32_t T = DIMT;
    const int32_t H = DIMH;

    // Allocate memory for matrices
    float* A = (float*)aligned_alloc(MEMALIGN, B_dim * T * H * sizeof(float));
    float* B = (float*)aligned_alloc(MEMALIGN, B_dim * H * T * sizeof(float));
    float* C = (float*)aligned_alloc(MEMALIGN, B_dim * T * T * sizeof(float));
    float* C_ref = (float*)aligned_alloc(MEMALIGN, B_dim * T * T * sizeof(float));

    if (!A || !B || !C || !C_ref) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // Initialize matrices
    init_rand(A, B_dim * T, H);
    init_rand(B, B_dim * H, T);
    init_const(C, 0.0f, B_dim * T, T);
    init_const(C_ref, 0.0f, B_dim * T, T);

    double FLOP = 2.0 * (double)B_dim * T * T * H;

    // Test looped SGEMM
    matmul_kernel = looped_sgemm;
    
    // Warmup runs
    for(int i = 0; i < WARMUP_ITER; i++) {
        matmul_kernel(A, B, C, B_dim, T, H);
    }

    // Timing runs
    double total_time = 0.0;
    for(int i = 0; i < NITER; i++) {
        uint64_t start = timer();
        matmul_kernel(A, B, C, B_dim, T, H);
        uint64_t end = timer();
        total_time += (end-start) * 1e-9;
    }
    
    double looped_time_ms = (total_time/NITER) * 1000;
    double looped_gflops = (FLOP / (total_time/NITER)) / 1e9;

    // Save reference output
    for(int i = 0; i < B_dim * T * T; i++) {
        C_ref[i] = C[i];
    }
    
    // Test BRGEMM
    /*matmul_kernel = brgemm_kernel;*/
    matmul_kernel = brgemm_kernel_strided;
    init_const(C, 0.0f, B_dim * T, T);
    
    // Warmup runs
    for(int i = 0; i < WARMUP_ITER; i++) {
        matmul_kernel(A, B, C, B_dim, T, H);
    }

    // Timing runs
    total_time = 0.0;
    for(int i = 0; i < NITER; i++) {
        uint64_t start = timer();
        matmul_kernel(A, B, C, B_dim, T, H);
        uint64_t end = timer();
        total_time += (end-start) * 1e-9;
    }

    double brgemm_time_ms = (total_time/NITER) * 1000;
    double brgemm_gflops = (FLOP / (total_time/NITER)) / 1e9;

    // Print results in CSV format
    printf("method,time_ms,gflops\n");
    printf("looped,%.3f,%.2f\n", looped_time_ms, looped_gflops);
    printf("brgemm,%.3f,%.2f\n", brgemm_time_ms, brgemm_gflops);

    // Verify results match
    compare_mats(C, C_ref, B_dim, T, T);

    free(A);
    free(B);
    free(C);
    free(C_ref);

    return 0;
}
