
#define NDEBUG

#include <assert.h>
#include <immintrin.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h> 
#include <time.h>

#define MEMALIGN 64

// defining dimensions
#define DIMM 4096
#define DIMN 4096
#define DIMK 4096

#ifndef NITER
    #define NITER 5
#endif /* ifndef NITER */

void init_rand(float* A, const int32_t M, const int32_t N){
    for(int i = 0; i < M*N; i++){
        *A++ = rand() / (float)RAND_MAX;
    }
}


void init_const(float* A, const float val, const int32_t M, const int32_t N){
    for(int i = 0; i < M*N; i++){
        *A++ = val;
    }
}

uint64_t timer() {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    return (uint64_t)start.tv_sec * 1000000000 + (uint64_t)start.tv_nsec;
}

void matmul_naive(float* A, float* B, float* C, const int32_t M, const int32_t N, const int32_t K)
{
    for(int i = 0; i < M; i++){
            for(int p = 0; p < K; p++){
        for(int j = 0; j < N; j++){
                C[i*M + j] += A[i*N + p] * B[p*K + j];
            }
        }
    }
}


void (*matmul_kernel)(float* A, float* B, float* C, const int32_t M, const int32_t N, const int32_t K);

int main(){
    matmul_kernel = matmul_naive;

    const int32_t M = DIMM;
    const int32_t N = DIMN;
    const int32_t K = DIMK; 

    // allocate memory for matrices
    float* A = (float*)_mm_malloc(M * K * sizeof(float), MEMALIGN);
    float* B = (float*)_mm_malloc(K * N * sizeof(float), MEMALIGN);
    float* C = (float*)_mm_malloc(M * N * sizeof(float), MEMALIGN);

    float* C_ref = (float*)_mm_malloc(M * N * sizeof(float), MEMALIGN);
    
    init_rand(A, M, K);
    init_rand(B, K, N);

    double FLOP = 2 * (double)M*K*N;

#ifdef TEST 
    matmul_naive(A, B, C_ref, M, N, K);
#endif

    for(int i=0; i<NITER; i++){
        init_const(C_ref, 0.0, M, N);
        uint64_t start = timer();
        matmul_kernel(A, B, C, M, N, K);
        uint64_t end = timer();

        double exec_time = (end-start) * 1e-9;
        double FLOPS = FLOP / exec_time;

        printf("Execution Time: %.3fms\n", exec_time * 1000);
        printf("GFLOPS: %.3f\n", FLOPS / 1e9);

    }

    _mm_free(A);
    _mm_free(B);
    _mm_free(C);
    _mm_free(C_ref);
}




