#include "kernel.h"
#include "omp.h"
#include <stdio.h>


#define min(x, y) ((x) < (y) ? (x) : (y))

#ifndef NTHREADS
    #define NTHREADS 1
#endif

#define MC (16 * NTHREADS * 2)
#define NC (6 * NTHREADS * 71)
#define KC 800

static float blockA_packed[MC * KC] __attribute__((aligned(64)));
static float blockB_packed[NC * KC] __attribute__((aligned(64)));

void pack_panelB(float* B, float* blockB_packed, int nr, int kc, int N) {
    for (int p = 0; p < kc; p++) {
        for (int j = 0; j < nr; j++) {
            *blockB_packed++ = B[p * N + j];
        }
        for (int j = nr; j < 16; j++) {
            *blockB_packed++ = 0;
        }
    }
}

void pack_blockB(float* B, float* blockB_packed, int nc, int kc, int N) {
#pragma omp parallel for num_threads(NTHREADS)
    for (int j = 0; j < nc; j += 16) {
        int nr = min(16, nc - j);
        pack_panelB(&B[j], &blockB_packed[j * kc], nr, kc, N);
    }
}


void pack_panelA(float* A, float* blockA_packed, int mr, int kc, int K) {
    for (int p = 0; p < kc; p++) {
        for (int i = 0; i < mr; i++) {
            #ifdef DEBUG
                printf("pack_panelA: i=%d, p=%d, index=%d, A_addr=%p, blockA_packed_addr=%p, mr=%d, kc=%d, K=%d\n", i, p, i*K + p, &A[i*K + p], blockA_packed, mr, kc, K);
            #endif /* ifdef DEBUG */
            *blockA_packed++ = A[i*K + p];
        }
        for (int i = mr; i < 6; i++) {
            *blockA_packed++ = 0;
        }
        #ifdef DEBUG
            printf("pack_panelA: is good here\n");
        #endif /* ifdef DEBUG */
    }
}

//reorder memory of A into a one-dimension array blockA_packed 
//for A, this copy process is strided
void pack_blockA(float* A, float* blockA_packed, int mc, int kc, int K) {
#pragma omp parallel for num_threads(NTHREADS)
    for (int i = 0; i < mc; i += 6) {
        int mr = min(6, mc - i);
        pack_panelA(&A[i * K], &blockA_packed[i * kc], mr, kc, K);
    }
}

void matmul(float* A, float* B, float* C, int M, int N, int K) {
    for(int i = 0; i < M; i += MC) {
        int mc = min(MC, M - i);
        for(int p = 0; p < K; p += KC){
            int kc = min(KC, K - p);
            //reorder MCxKC block of A starting from i*k + p 
            pack_blockA(&A[i * K + p], blockA_packed, mc, kc, K);
                        #ifdef DEBUG
                            printf("Matrix A is packed\n");
                        #endif /* ifdef DEBUG */

            for(int j = 0; j < N; j += NC) {
                int nc = min(NC, N - j);
                //reordere KCxNC block of A starting from p*n + j
                pack_blockB(&B[p * N + j], blockB_packed, nc, kc, N);
                        #ifdef DEBUG
                            printf("Matrix B is packed\n");
                        #endif /* ifdef DEBUG */


/*#pragma omp parallel for collapse(2) num_threads(NTHREADS)*/
                //inside the block of MCxNC work on a panel of MRxNR 
                for(int ir = 0; ir < mc; ir += 6) {
                    for(int jr = 0; jr < nc; jr += 16) {
                        int mr = min(6, mc - ir);
                        int nr = min(16, nc - jr);
                        kernel_16x6(&blockA_packed[ir * kc],
                                    &blockB_packed[jr * kc],
                                    &C[(i + ir)*N + (j + jr)],
                                    mr,
                                    nr,
                                    kc,
                                    N);
                        #ifdef DEBUG
                            printf("Status=> ir/mc: %i/%i, jr/nc: %i/%i\n", ir, mc, jr, nc);
                            if(ir == mc -1) printf("\n");
                        #endif /* ifdef DEBUG */
                    }
                }
                        #ifdef DEBUG
                            printf("working of the Block is done!\n");
                        #endif /* ifdef DEBUG */
            }
        }
    }
}

void matmul_naive(float* A, float* B, float* C, int m, int n, int k) {
    //kij loop order
#pragma omp parallel for num_threads(NTHREADS) collapse(2)
    for (int i = 0; i < m; i++) {
            for (int p = 0; p < k; p++) {
        for (int j = 0; j < n; j++) {
                C[i * m + j] += A[i * m + p] * B[p * k + j];
            }
        }
    }
}
