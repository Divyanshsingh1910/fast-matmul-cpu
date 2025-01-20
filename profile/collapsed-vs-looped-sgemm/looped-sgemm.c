/*
 gcc -O3 -march=native -I${MKLROOT}/include     looped-sgemm.c     -L${MKLROOT}/lib/intel64     -Wl,--no-as-needed -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl -fopenmp    -o matmul_test
*/
#define NDEBUG

#include <assert.h>
#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h> 
#include <stdlib.h>
#include <time.h>
#include <cblas.h>
/*#include <mkl_cblas.h>*/

#define MEMALIGN 64

// Matrix dimensions
#ifndef DIMM
    #define DIMM 1024
#endif
#ifndef DIMN
#define DIMN 4096
#endif
#ifndef DIMK
#define DIMK 4096
#endif

#ifndef BATCH
    #define BATCH 1
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

void compare_mats(float* mat1, float* mat2, const int M, const int N) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            if (fabsf(mat1[j * M + i] - mat2[j * M + i]) > 1e-3) {
                printf("MISMATCH! Element[%d][%d] %f != %f\n",
                       i,
                       j,
                       mat1[j * M + i],
                       mat2[j * M + i]);
                return;
            }
        }
    }
    printf("MATCH!\n");
    return;
}

void matmul_naive(float* A, float* B, float* C, const int32_t M, const int32_t N, const int32_t K) {
    for(int b = 0; b < BATCH; b++) {
        float* C_batch = C + b * M * N;
        float* A_batch = A + b * M * K;
        
        for(int i = 0; i < M; i++) {
            for(int j = 0; j < N; j++) {
                float sum = 0.0f;
                for(int p = 0; p < K; p++) {
                    sum += A_batch[i*K + p] * B[p*N + j];
                }
                C_batch[i*N + j] = sum;
            }
        }
    }
}

void looped_sgemm(float* A, float* B, float* C, const int32_t M, const int32_t N, const int32_t K) {
    /*
    * A: (BATCH, M, K)
    * B: (K, N)
    * C: (BATCH, M, N)
    *
    * Op --> C = alpha * (A @ B) + beta * C
    */ 
    const float alpha = 1.0f;
    const float beta = 0.0f;
    
    for (int b = 0; b < BATCH; b++) {
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    M, N, K,
                    alpha,
                    A + b * M * K, K,    // stride to next batch in A
                    B, N,                 // B is shared across batches
                    beta,
                    C + b * M * N, N);    // stride to next batch in C
    }
}


void sgemm(float* A, float* B, float* C,  int32_t M, const int32_t N, const int32_t K) {
    /*
    * A: (BATCH, M, K)
    * B: (K, N)
    * C: (BATCH, M, N)
    *
    * Op --> C = alpha * (A @ B) + beta * C
    */ 
    const float alpha = 1.0f;
    const float beta = 0.0f;
    
    M *= BATCH;

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                M, N, K,
                alpha,
                A , K,    // stride to next batch in A
                B, N,                 // B is shared across batches
                beta,
                C , N);    // stride to next batch in C
}

void (*matmul_kernel)(float* A, float* B, float* C, const int32_t M, const int32_t N, const int32_t K);

int main() {
    matmul_kernel = sgemm;
    srand(1);  // For reproducible results
    
    const int32_t M = DIMM;
    const int32_t N = DIMN;
    const int32_t K = DIMK; 

    // Allocate memory for matrices
    float* A = (float*)aligned_alloc(MEMALIGN, BATCH * M * K * sizeof(float));
    float* B = (float*)aligned_alloc(MEMALIGN, K * N * sizeof(float));
    float* C = (float*)aligned_alloc(MEMALIGN, BATCH * M * N * sizeof(float));

    float* C_ref = (float*)aligned_alloc(MEMALIGN, BATCH * M * N * sizeof(float));

    if (!A || !B || !C) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // Initialize matrices
    init_rand(A, BATCH * M, K);
    init_rand(B, K, N);
    init_const(C, 0.0f, BATCH * M, N);

    double FLOP = 2.0 * (double)M * N * K * BATCH;  // multiply-add per matrix multiply
    printf("FLOP: %.3f\n", FLOP);

    // Warmup runs
    printf("Performing warmup runs...\n");
    for(int i = 0; i < WARMUP_ITER; i++) {
        matmul_kernel(A, B, C, M, N, K);
    }

    // Timing runs
    printf("Performing timing runs...\n");
    double total_time = 0.0;
    
    for(int i = 0; i < NITER; i++) {
        uint64_t start = timer();
        matmul_kernel(A, B, C, M, N, K);
        uint64_t end = timer();

        double exec_time = (end-start) * 1e-9;
        double FLOPS = FLOP / exec_time;
        total_time += exec_time;

        printf("Run %d:\n", i+1);
        printf("  Execution Time: %.3f ms\n", exec_time * 1000);
        printf("  Performance: %.2f GFLOPS\n", FLOPS / 1e9);
    }

    printf("\nAverage over %d runs: %.3f ms (%.2f GFLOPS)\n", 
           NITER, 
           (total_time/NITER) * 1000, 
           (FLOP / (total_time/NITER)) / 1e9);

#ifdef TEST
    sgemm(A, B, C_ref, M, N, K);
    compare_mats(C, C_ref, BATCH*M, N);
#endif /* ifdef TEST */
    free(A);
    free(B);
    free(C);

    return 0;
}
