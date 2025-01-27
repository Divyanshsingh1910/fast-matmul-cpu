#include "kernel.h"

void kernel_16x6(float* blockA_packed,
                 float* blockB_packed,
                 float* C,
                 int mr,
                 int nr,
                 int kc,
                 int n) {
    __m256 C00 = _mm256_setzero_ps();
    __m256 C01 = _mm256_setzero_ps();
    __m256 C10 = _mm256_setzero_ps();
    __m256 C11 = _mm256_setzero_ps();
    __m256 C20 = _mm256_setzero_ps();
    __m256 C21 = _mm256_setzero_ps();
    __m256 C30 = _mm256_setzero_ps();
    __m256 C31 = _mm256_setzero_ps();
    __m256 C40 = _mm256_setzero_ps();
    __m256 C41 = _mm256_setzero_ps();
    __m256 C50 = _mm256_setzero_ps();
    __m256 C51 = _mm256_setzero_ps();


    __m256 a_packFloat8;
    __m256 b0_packFloat8;
    __m256 b1_packFloat8;

    /*
     *                   Layout
     *                ------------
     *  c00 c01             |             --b0_packFloat8--|--b1_packFloat8-- 
     *  c10 c11             |
     *  c20 c21     =  a_packFloat8   *  
     *  .....               |
     *  c50 c51             |    
     *
    */


    __m256i packed_mask0;
    __m256i packed_mask1;

    if (nr != 16) {
        packed_mask0 = _mm256_cvtepi8_epi32(_mm_loadu_si64(&mask[16 - mr]));
        packed_mask1 = _mm256_cvtepi8_epi32(_mm_loadu_si64(&mask[16 - mr + 8]));
        switch (mr) {
        case 1 :
            C00 = _mm256_maskload_ps(C, packed_mask0); 
            C01 = _mm256_maskload_ps(&C[8], packed_mask1);
            break;
        case 2 :
            C00 = _mm256_maskload_ps(C, packed_mask0); 
            C01 = _mm256_maskload_ps(&C[8], packed_mask1);
            C10 = _mm256_maskload_ps(&C[n], packed_mask0);
            C11 = _mm256_maskload_ps(&C[n + 8], packed_mask1);
            break;
        case 3 :
            C00 = _mm256_maskload_ps(C, packed_mask0); 
            C01 = _mm256_maskload_ps(&C[8], packed_mask1);
            C10 = _mm256_maskload_ps(&C[n], packed_mask0);
            C11 = _mm256_maskload_ps(&C[n + 8], packed_mask1);
            C20 = _mm256_maskload_ps(&C[2 * n], packed_mask0);
            C21 = _mm256_maskload_ps(&C[2 * n + 8], packed_mask1);
            break;
        case 4 :
            C00 = _mm256_maskload_ps(C, packed_mask0); 
            C01 = _mm256_maskload_ps(&C[8], packed_mask1);
            C10 = _mm256_maskload_ps(&C[n], packed_mask0);
            C11 = _mm256_maskload_ps(&C[n + 8], packed_mask1);
            C20 = _mm256_maskload_ps(&C[2 * n], packed_mask0);
            C21 = _mm256_maskload_ps(&C[2 * n + 8], packed_mask1);
            C30 = _mm256_maskload_ps(&C[3 * n], packed_mask0);
            C31 = _mm256_maskload_ps(&C[3 * n + 8], packed_mask1);
            break;
        case 5 :
            C00 = _mm256_maskload_ps(C, packed_mask0); 
            C01 = _mm256_maskload_ps(&C[8], packed_mask1);
            C10 = _mm256_maskload_ps(&C[n], packed_mask0);
            C11 = _mm256_maskload_ps(&C[n + 8], packed_mask1);
            C20 = _mm256_maskload_ps(&C[2 * n], packed_mask0);
            C21 = _mm256_maskload_ps(&C[2 * n + 8], packed_mask1);
            C30 = _mm256_maskload_ps(&C[3 * n], packed_mask0);
            C31 = _mm256_maskload_ps(&C[3 * n + 8], packed_mask1);
            C40 = _mm256_maskload_ps(&C[4 * n], packed_mask0);
            C41 = _mm256_maskload_ps(&C[4 * n + 8], packed_mask1);
            break;
        case 6 :
            C00 = _mm256_maskload_ps(C, packed_mask0); 
            C01 = _mm256_maskload_ps(&C[8], packed_mask1);
            C10 = _mm256_maskload_ps(&C[n], packed_mask0);
            C11 = _mm256_maskload_ps(&C[n + 8], packed_mask1);
            C20 = _mm256_maskload_ps(&C[2 * n], packed_mask0);
            C21 = _mm256_maskload_ps(&C[2 * n + 8], packed_mask1);
            C30 = _mm256_maskload_ps(&C[3 * n], packed_mask0);
            C31 = _mm256_maskload_ps(&C[3 * n + 8], packed_mask1);
            C40 = _mm256_maskload_ps(&C[4 * n], packed_mask0);
            C41 = _mm256_maskload_ps(&C[4 * n + 8], packed_mask1);
            C50 = _mm256_maskload_ps(&C[5 * n], packed_mask0);
            C51 = _mm256_maskload_ps(&C[5 * n + 8], packed_mask1);
            break;
        }
    } else {
        switch (mr) {
        case 1 :
            C00 = _mm256_loadu_ps(C);
            C01 = _mm256_loadu_ps(&C[8]);
            break;
        case 2 :
            C00 = _mm256_loadu_ps(C);
            C01 = _mm256_loadu_ps(&C[8]);
            C10 = _mm256_loadu_ps(&C[n]);
            C11 = _mm256_loadu_ps(&C[n + 8]);
            break;
        case 3 :
            C00 = _mm256_loadu_ps(C);
            C01 = _mm256_loadu_ps(&C[8]);
            C10 = _mm256_loadu_ps(&C[n]);
            C11 = _mm256_loadu_ps(&C[n + 8]);
            C20 = _mm256_loadu_ps(&C[2 * n]);
            C21 = _mm256_loadu_ps(&C[2 * n + 8]);
            break;
        case 4 :
            C00 = _mm256_loadu_ps(C);
            C01 = _mm256_loadu_ps(&C[8]);
            C10 = _mm256_loadu_ps(&C[n]);
            C11 = _mm256_loadu_ps(&C[n + 8]);
            C20 = _mm256_loadu_ps(&C[2 * n]);
            C21 = _mm256_loadu_ps(&C[2 * n + 8]);
            C30 = _mm256_loadu_ps(&C[3 * n]);
            C31 = _mm256_loadu_ps(&C[3 * n + 8]);
            break;
        case 5 :
            C00 = _mm256_loadu_ps(C);
            C01 = _mm256_loadu_ps(&C[8]);
            C10 = _mm256_loadu_ps(&C[n]);
            C11 = _mm256_loadu_ps(&C[n + 8]);
            C20 = _mm256_loadu_ps(&C[2 * n]);
            C21 = _mm256_loadu_ps(&C[2 * n + 8]);
            C30 = _mm256_loadu_ps(&C[3 * n]);
            C31 = _mm256_loadu_ps(&C[3 * n + 8]);
            C40 = _mm256_loadu_ps(&C[4 * n]);
            C41 = _mm256_loadu_ps(&C[4 * n + 8]);
            break;
        case 6 :
            C00 = _mm256_loadu_ps(C);
            C01 = _mm256_loadu_ps(&C[8]);
            C10 = _mm256_loadu_ps(&C[n]);
            C11 = _mm256_loadu_ps(&C[n + 8]);
            C20 = _mm256_loadu_ps(&C[2 * n]);
            C21 = _mm256_loadu_ps(&C[2 * n + 8]);
            C30 = _mm256_loadu_ps(&C[3 * n]);
            C31 = _mm256_loadu_ps(&C[3 * n + 8]);
            C40 = _mm256_loadu_ps(&C[4 * n]);
            C41 = _mm256_loadu_ps(&C[4 * n + 8]);
            C50 = _mm256_loadu_ps(&C[5 * n]);
            C51 = _mm256_loadu_ps(&C[5 * n + 8]);
            break;
        }
    }
    for (int p = 0; p < kc; p++) {
        b0_packFloat8 = _mm256_loadu_ps(blockB_packed);
        b1_packFloat8 = _mm256_loadu_ps(blockB_packed + 8);

        a_packFloat8 = _mm256_broadcast_ss(blockA_packed);
        C00 = _mm256_fmadd_ps(b0_packFloat8, a_packFloat8, C00);
        C01 = _mm256_fmadd_ps(b1_packFloat8, a_packFloat8, C01);

        a_packFloat8 = _mm256_broadcast_ss(blockA_packed + 1);
        C10 = _mm256_fmadd_ps(b0_packFloat8, a_packFloat8, C10);
        C11 = _mm256_fmadd_ps(b1_packFloat8, a_packFloat8, C11);

        a_packFloat8 = _mm256_broadcast_ss(blockA_packed + 2);
        C20 = _mm256_fmadd_ps(b0_packFloat8, a_packFloat8, C20);
        C21 = _mm256_fmadd_ps(b1_packFloat8, a_packFloat8, C21);

        a_packFloat8 = _mm256_broadcast_ss(blockA_packed + 3);
        C30 = _mm256_fmadd_ps(b0_packFloat8, a_packFloat8, C30);
        C31 = _mm256_fmadd_ps(b1_packFloat8, a_packFloat8, C31);

        a_packFloat8 = _mm256_broadcast_ss(blockA_packed + 4);
        C40 = _mm256_fmadd_ps(b0_packFloat8, a_packFloat8, C40);
        C41 = _mm256_fmadd_ps(b1_packFloat8, a_packFloat8, C41);

        a_packFloat8 = _mm256_broadcast_ss(blockA_packed + 5);
        C50 = _mm256_fmadd_ps(b0_packFloat8, a_packFloat8, C50);
        C51 = _mm256_fmadd_ps(b1_packFloat8, a_packFloat8, C51);

        blockA_packed += 16;
        blockB_packed += 6;
    }
    if (nr != 16) {
        switch (mr) {
        case 1 :
            _mm256_maskstore_ps(C, packed_mask0, C00);
            _mm256_maskstore_ps(&C[8], packed_mask1, C01);
            break;
        case 2 :
            _mm256_maskstore_ps(C, packed_mask0, C00);
            _mm256_maskstore_ps(&C[8], packed_mask1, C01);
            _mm256_maskstore_ps(&C[n], packed_mask0, C10);
            _mm256_maskstore_ps(&C[n + 8], packed_mask1, C11);
            break;
        case 3 :
            _mm256_maskstore_ps(C, packed_mask0, C00);
            _mm256_maskstore_ps(&C[8], packed_mask1, C01);
            _mm256_maskstore_ps(&C[n], packed_mask0, C10);
            _mm256_maskstore_ps(&C[n + 8], packed_mask1, C11);
            _mm256_maskstore_ps(&C[2 * n], packed_mask0, C20);
            _mm256_maskstore_ps(&C[2 * n + 8], packed_mask1, C21);
            break;
        case 4 :
            _mm256_maskstore_ps(C, packed_mask0, C00);
            _mm256_maskstore_ps(&C[8], packed_mask1, C01);
            _mm256_maskstore_ps(&C[n], packed_mask0, C10);
            _mm256_maskstore_ps(&C[n + 8], packed_mask1, C11);
            _mm256_maskstore_ps(&C[2 * n], packed_mask0, C20);
            _mm256_maskstore_ps(&C[2 * n + 8], packed_mask1, C21);
            _mm256_maskstore_ps(&C[3 * n], packed_mask0, C30);
            _mm256_maskstore_ps(&C[3 * n + 8], packed_mask1, C31);
            break;
        case 5 :
            _mm256_maskstore_ps(C, packed_mask0, C00);
            _mm256_maskstore_ps(&C[8], packed_mask1, C01);
            _mm256_maskstore_ps(&C[n], packed_mask0, C10);
            _mm256_maskstore_ps(&C[n + 8], packed_mask1, C11);
            _mm256_maskstore_ps(&C[2 * n], packed_mask0, C20);
            _mm256_maskstore_ps(&C[2 * n + 8], packed_mask1, C21);
            _mm256_maskstore_ps(&C[3 * n], packed_mask0, C30);
            _mm256_maskstore_ps(&C[3 * n + 8], packed_mask1, C31);
            _mm256_maskstore_ps(&C[4 * n], packed_mask0, C40);
            _mm256_maskstore_ps(&C[4 * n + 8], packed_mask1, C41);
            break;
        case 6 :
            _mm256_maskstore_ps(C, packed_mask0, C00);
            _mm256_maskstore_ps(&C[8], packed_mask1, C01);
            _mm256_maskstore_ps(&C[n], packed_mask0, C10);
            _mm256_maskstore_ps(&C[n + 8], packed_mask1, C11);
            _mm256_maskstore_ps(&C[2 * n], packed_mask0, C20);
            _mm256_maskstore_ps(&C[2 * n + 8], packed_mask1, C21);
            _mm256_maskstore_ps(&C[3 * n], packed_mask0, C30);
            _mm256_maskstore_ps(&C[3 * n + 8], packed_mask1, C31);
            _mm256_maskstore_ps(&C[4 * n], packed_mask0, C40);
            _mm256_maskstore_ps(&C[4 * n + 8], packed_mask1, C41);
            _mm256_maskstore_ps(&C[5 * n], packed_mask0, C50);
            _mm256_maskstore_ps(&C[5 * n + 8], packed_mask1, C51);
            break;
        }
    } else {
        switch (mr) {
        case 1 :
            _mm256_storeu_ps(C, C00);
            _mm256_storeu_ps(&C[8], C01);
            break;
        case 2 :
            _mm256_storeu_ps(C, C00);
            _mm256_storeu_ps(&C[8], C01);
            _mm256_storeu_ps(&C[n], C10);
            _mm256_storeu_ps(&C[n + 8], C11);
            break;
        case 3 :
            _mm256_storeu_ps(C, C00);
            _mm256_storeu_ps(&C[8], C01);
            _mm256_storeu_ps(&C[n], C10);
            _mm256_storeu_ps(&C[n + 8], C11);
            _mm256_storeu_ps(&C[2 * n], C20);
            _mm256_storeu_ps(&C[2 * n + 8], C21);
            break;
        case 4 :
            _mm256_storeu_ps(C, C00);
            _mm256_storeu_ps(&C[8], C01);
            _mm256_storeu_ps(&C[n], C10);
            _mm256_storeu_ps(&C[n + 8], C11);
            _mm256_storeu_ps(&C[2 * n], C20);
            _mm256_storeu_ps(&C[2 * n + 8], C21);
            _mm256_storeu_ps(&C[3 * n], C30);
            _mm256_storeu_ps(&C[3 * n + 8], C31);
            break;
        case 5 :
            _mm256_storeu_ps(C, C00);
            _mm256_storeu_ps(&C[8], C01);
            _mm256_storeu_ps(&C[n], C10);
            _mm256_storeu_ps(&C[n + 8], C11);
            _mm256_storeu_ps(&C[2 * n], C20);
            _mm256_storeu_ps(&C[2 * n + 8], C21);
            _mm256_storeu_ps(&C[3 * n], C30);
            _mm256_storeu_ps(&C[3 * n + 8], C31);
            _mm256_storeu_ps(&C[4 * n], C40);
            _mm256_storeu_ps(&C[4 * n + 8], C41);
            break;
        case 6 :
            _mm256_storeu_ps(C, C00);
            _mm256_storeu_ps(&C[8], C01);
            _mm256_storeu_ps(&C[n], C10);
            _mm256_storeu_ps(&C[n + 8], C11);
            _mm256_storeu_ps(&C[2 * n], C20);
            _mm256_storeu_ps(&C[2 * n + 8], C21);
            _mm256_storeu_ps(&C[3 * n], C30);
            _mm256_storeu_ps(&C[3 * n + 8], C31);
            _mm256_storeu_ps(&C[4 * n], C40);
            _mm256_storeu_ps(&C[4 * n + 8], C41);
            _mm256_storeu_ps(&C[5 * n], C50);
            _mm256_storeu_ps(&C[5 * n + 8], C51);
            break;
        }
    }
}
