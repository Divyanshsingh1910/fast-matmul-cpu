/*
 * Singl-threaded flags: 
        gcc matmul.c -o matmul \
            -I/usr/include/mkl \
            -L/usr/lib/x86_64-linux-gnu \
            -Wl,--no-as-needed -lmkl_intel_lp64 \ 
            -lmkl_sequential \ 
            -lmkl_core -lpthread -lm

 * Multi-threaded flags:
        gcc matmul.c -o matmul \     
            -I/usr/include/mkl \ 
            -L/usr/lib/x86_64-linux-gnu \     
            -Wl,--no-as-needed -lmkl_intel_lp64 \
            -lmkl_gnu_thread \ 
            -lmkl_core -lpthread -lm \ 
            -lgomp -fopenmp \
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <mkl.h>

// Matrix dimensions
#ifndef M
    #define M 4096
    #define N 4096
    #define K 4096
#endif 

#ifndef NITER
    #define NITER 5
#endif

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
    double *A, *B, *C;
    double alpha = 1.0, beta = 0.0;
    double start_time, end_time, elapsed_time;
    double gflops;
    double GFLOPS = 0.0, TOT_TIME = 0.0;

    // Allocate memory for matrices
    A = (double*)mkl_malloc(M * K * sizeof(double), 64);
    B = (double*)mkl_malloc(K * N * sizeof(double), 64);
    C = (double*)mkl_malloc(M * N * sizeof(double), 64);

    if (A == NULL || B == NULL || C == NULL) {
        printf("Memory allocation failed!\n");
        mkl_free(A);
        mkl_free(B);
        mkl_free(C);
        return 1;
    }

    // Initialize matrices with random values
    for (int i = 0; i < M * K; i++) {
        A[i] = (double)rand() / RAND_MAX;
    }
    for (int i = 0; i < K * N; i++) {
        B[i] = (double)rand() / RAND_MAX;
    }
    for (int i = 0; i < M * N; i++) {
        C[i] = 0.0;
    }

    printf("warm up run...\n");
    // Warm up run
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                M, N, K,
                alpha,
                A, K,  // lda = K for row-major
                B, N,  // ldb = N for row-major
                beta,
                C, N); // ldc = N for row-major

    // Actual timing run
    for(int i=0; i<NITER; i++){
        start_time = get_time();

        cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    M, N, K,
                    alpha,
                    A, K,
                    B, N,
                    beta,
                    C, N);

        end_time = get_time();
        elapsed_time = end_time - start_time;
        TOT_TIME += elapsed_time;

        // Calculate GFLOPS
        // For matrix multiplication, FLOPS = 2 * M * N * K
        gflops = (2.0 * M * N * K) / (elapsed_time * 1e9);
        GFLOPS += gflops;
    }

    printf("Matrix dimensions: %d x %d x %d\n", M, N, K);
    printf("Time elapsed: %.3f seconds\n", TOT_TIME / NITER);
    printf("Performance: %.2f GFLOPS\n", GFLOPS / NITER);

    // Free memory
    mkl_free(A);
    mkl_free(B);
    mkl_free(C);

    return 0;
}
