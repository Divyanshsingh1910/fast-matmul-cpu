#define _POSIX_C_SOURCE 199309L
#include "helper_matrix.h"
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "matmul.h"

#define MEMALIGN 64

uint64_t timer() {
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    return (uint64_t)start.tv_sec * 1000000000 + (uint64_t)start.tv_nsec;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    int MINSIZE = 96;
    int MAXSIZE = 6144;
    int NPTS = 40;
    int WARMUP = 1;

    if (argc > 4) {
        MINSIZE = atoi(argv[1]);
        MAXSIZE = atoi(argv[2]);
        NPTS = atoi(argv[3]);
        WARMUP = atoi(argv[4]);
    }

    printf("================\n");
    printf("MINSIZE = %i\nMAXSIZE = %i\nNPTS = %i\nWARMUP = %i\n", MINSIZE, MAXSIZE, NPTS, WARMUP);
    printf("================\n");
    int avg_gflops[NPTS];
    int min_gflops[NPTS];
    int max_gflops[NPTS];
    int matsizes[NPTS];

    double delta_size = (double)(MAXSIZE - MINSIZE) / (NPTS - 1);
    for (int i = 0; i < NPTS - 1; i++) {
        matsizes[i] = MINSIZE + i * delta_size;
    }
    matsizes[NPTS - 1] = MAXSIZE;

    printf("Allocating the memories...\n");
    float* A = (float*)_mm_malloc(MAXSIZE * MAXSIZE * sizeof(float), MEMALIGN);
    float* B = (float*)_mm_malloc(MAXSIZE * MAXSIZE * sizeof(float), MEMALIGN);
    float* C = (float*)_mm_malloc(MAXSIZE * MAXSIZE * sizeof(float), MEMALIGN);
    float* C_ref = (float*)_mm_malloc(MAXSIZE * MAXSIZE * sizeof(float), MEMALIGN);
    init_rand(A, MAXSIZE, MAXSIZE);
    init_rand(B, MAXSIZE, MAXSIZE);
    printf("Memory allocated succesffully!\n");
    for (int j = 0; j < WARMUP; j++) {
        fflush(stdout);
        init_const(C, 0.0, MAXSIZE, MAXSIZE);
        int m = MAXSIZE;
        int n = MAXSIZE;
        int k = MAXSIZE;
        matmul(A, B, C, m, n, k);

        matmul_naive(A, B, C_ref, m, n, k);
        struct cmp_result result = compare_mats(C, C_ref, m, n); 
        printf("Results: #false: %i, #nans: %i, #inf: %i, avg_diff: %f\n\n", result.n_false, result.n_nans, result.n_inf, (double)result.tot_diff/result.n_false);
        
        #ifdef DEBUG
            printf("A matrix: \n");
            print_mat(A, m, n); 
            printf("B matrix: \n");
            print_mat(B, m, n); 
            printf("Reference matrix: \n");
            print_mat(C_ref, m, n); 
            printf("Calculated matrix: \n");
            print_mat(C, m, n); 
        #endif
    }

    printf("\n");
    printf("Matmul done\n");
    _mm_free(A);
    _mm_free(B);
    _mm_free(C);
    return 0;
}
