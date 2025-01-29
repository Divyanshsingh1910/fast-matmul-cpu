
#define _POSIX_C_SOURCE 199309L
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

void print_m256i_binary(__m256i vec) {
    uint32_t chunks[8];
    _mm256_storeu_si256((__m256i*)chunks, vec);
    
    for(int i=0; i<8; i++) printf("%d ", chunks[i]);
    printf("\n");
}


int main(int argc, char* argv[]) {

    static int8_t mask[32]
        __attribute__((aligned(64))) = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

    int mr = atoi(argv[1]);

    __m256i vmask = _mm256_cvtepi8_epi32(_mm_loadu_si64(&mask[16 - mr]));
    __m256i vmask1 = _mm256_cvtepi8_epi32(_mm_loadu_si64(&mask[16 - mr + 8]));

    print_m256i_binary(vmask);    
    print_m256i_binary(vmask1);    
    
    return 0; 
}
