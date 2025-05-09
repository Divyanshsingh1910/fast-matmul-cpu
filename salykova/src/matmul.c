#include "kernel.h"
#include "omp.h"
#include <stdio.h>
#include "helper_matrix.h"


#define min(x, y) ((x) < (y) ? (x) : (y))

#ifndef NTHREADS
    #define NTHREADS 12
#endif
#ifndef MR
    #define MR 6
#endif
#ifndef NR
    #define NR 32
#endif

#ifndef MC
	#define MC (MR * NTHREADS * 71)
#endif
#ifndef NC
	#define NC (NR * NTHREADS * 2)
#endif
#ifndef KC
    #define KC 600
#endif

static float blockA_packed[MC * KC] __attribute__((aligned(64)));
static float blockB_packed[NC * KC] __attribute__((aligned(64)));

void pack_panelB(float* B, float* blockB_packed, int nr, int kc, int N) {
    for (int p = 0; p < kc; p++) {
        for (int j = 0; j < nr; j++) {
            *blockB_packed++ = B[p * N + j];
        }
        for (int j = nr; j < NR; j++) {
            *blockB_packed++ = 0;
        }
    }
}

void pack_blockB(float* B, float* blockB_packed, int nc, int kc, int N) {
#pragma omp parallel for num_threads(NTHREADS)
    for (int j = 0; j < nc; j += NR) {
        int nr = min(NR, nc - j);
        pack_panelB(&B[j], &blockB_packed[j * kc], nr, kc, N);
    }
}


void pack_panelA(float* A, float* blockA_packed, int mr, int kc, int K) {
    for (int p = 0; p < kc; p++) {
        for (int i = 0; i < mr; i++) {
            *blockA_packed++ = A[i*K + p];
        }
        for (int i = mr; i < MR; i++) {
            *blockA_packed++ = 0;
        }
    }
}
//reorder memory of A into a one-dimension array blockA_packed 
//for A, this copy process is strided
void pack_blockA(float* A, float* blockA_packed, int mc, int kc, int K) {
#pragma omp parallel for num_threads(NTHREADS)
    for (int i = 0; i < mc; i += MR) {
        int mr = min(MR, mc - i);
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
                            //print_mat(blockA_packed, mc*kc, 1); 
                        #endif /* ifdef DEBUG */

            for(int j = 0; j < N; j += NC) {
                int nc = min(NC, N - j);
                //reordere KCxNC block of A starting from p*n + j
                pack_blockB(&B[p * N + j], blockB_packed, nc, kc, N);
                        #ifdef DEBUG
                            printf("Matrix B is packed\n");
                        #endif /* ifdef DEBUG */


#pragma omp parallel for collapse(2) num_threads(NTHREADS)
                //inside the block of MCxNC work on a panel of MRxNR 
                /*printf("mc=%i\tnc=%i\tkc=%i\n", mc, nc, kc);*/
                for(int ir = 0; ir < mc; ir += MR) {
                    for(int jr = 0; jr < nc; jr += NR) {
                        int mr = min(MR, mc - ir); //16 
                        int nr = min(NR, nc - jr); //6

                        // 4x4 ==> 16x16 ==> 16 --> dist ovoer omp theread 
                        // per thread = 16 / 4  
#ifdef AVX512
                        kernel_32x6_512(&blockA_packed[ir * kc],
                                    &blockB_packed[jr * kc],
                                    &C[(i + ir)*N + (j + jr)],
                                    mr,
                                    nr,
                                    kc,
                                    N);
#else
                        kernel_16x6(&blockA_packed[ir * kc],
                                    &blockB_packed[jr * kc],
                                    &C[(i + ir)*N + (j + jr)],
                                    mr,
                                    nr,
                                    kc,
                                    N);
#endif
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
                C[i * n + j] += A[i * k + p] * B[p * n + j];
            }
        }
    }
}
