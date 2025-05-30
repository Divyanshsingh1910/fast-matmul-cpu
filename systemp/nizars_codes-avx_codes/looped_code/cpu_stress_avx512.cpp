//#include "/usr/lib/gcc/x86_64-redhat-linux/4.8.5/include/omp.h" // for omp functions and pragma
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>   // for errno
#include <limits.h>  // for INT_MAX
#include <stdlib.h>  // for strtol
#include <ctime> //To measure time
#include <sys/time.h>
#include <immintrin.h>
#include <math.h>

const int N = 83040;

double slow_dot_product(const double *a, const double *b) {
  double answer = 0.0;
  for(int ii = 0; ii < N; ++ii)
    answer += a[ii]*b[ii];
  return answer;
}

/* Horizontal add works within 128-bit lanes. Use scalar ops to add
 *  * across the boundary. */
double reduce_vector1(__m256d input) {
  __m256d temp = _mm256_hadd_pd(input, input); // horizontal add
  return ((double*)&temp)[0] + ((double*)&temp)[2];
}

/* Another way to get around the 128-bit boundary: grab the first 128
 *  * bits, grab the lower 128 bits and then add them together with a 128
 *   * bit add instruction. */
/*
double reduce_vector2(__m512d input) {
  __m512d temp = _mm512_hadd_pd(input, input);
  __m256d sum_high = _mm512_extractf128_pd(temp, 1); //get the highest 128 bits
  __m256d result = _mm_add_pd(sum_high, _mm512_castpd256_pd128(temp)); // add the higest 128 bit with the lowerst 128 bits
  return ((double*)&result)[0];
}
*/
double reduce_vector0(__m256d input) {
  __m256d temp = _mm256_hadd_pd(input, input);
  __m128d sum_high = _mm256_extractf128_pd(temp, 1); //get the highest 128 bits
  __m128d result = _mm_add_pd(sum_high, _mm256_castpd256_pd128(temp)); // add the higest 128 bit with the lowerst 128 bits
  return ((double*)&result)[0];
}
double dot_product(const double *a, const double *b) {
  __m512d sum_vec = _mm512_set_pd(0.0, 0.0, 0.0, 0.0 , 0.0,0.0,0.0,0.0 ); //set 256 avx varaible with 4 doubles equal to zerpo
  double final;
  /* Add up partial dot-products in blocks of 256 bits */
  for(int ii = 0; ii < N/8; ++ii) {
    __m512d x = _mm512_load_pd(a+8*ii); //load variaible
    __m512d y = _mm512_load_pd(b+8*ii); //load varaible
    __m512d z = _mm512_mul_pd(x,y); // multiply vector
    sum_vec = _mm512_add_pd(sum_vec, z); //sum original vector with multiplication results
  }

  /* Find the partial dot-product for the remaining elements after
 *    * dealing with all 256-bit blocks. */
 // for(int ii = N-N%8; ii < N; ++ii)
    final = _mm512_reduce_add_pd(sum_vec);

  return  final;
}
#ifdef NOOMP
/*
int omp_set_num_threads( int i){
return 1;}
int omp_get_thread_num(){
 	return 1;}
int omp_get_num_threads(){
	 return 1;}
*/
#endif
using namespace std;

int test_function()
{
int d = 0;
for(int j =0; j<20000000;j++)
        {
          int a =j;
          int b =j+2;
          int c = a+b *(a*b);
          d += a*a+b*b+c*c;

        }
return d;
}
float res_array[112];
int main( int argc, char **argv){
int nthreads, tid;
char *p;
int num;
long conv;
double delta, res;
struct timeval start, end;
__attribute__ ((aligned (32))) double a[N], b[N]; 
 
   for(int ii = 0; ii < N; ++ii) 
     a[ii] = b[ii] = ii/sqrt(N); 

gettimeofday(&start, NULL);
// read command line input, if available read the first input as integer and use it to set the number of threads 
// if no input is given we allow a dynamic number of threads to be set by the kernel
if (argc > 1)
	{	conv = strtol(argv[1], &p, 10); //read the input from the command line
		num = conv; // transform to int
		omp_set_num_threads(num); // reques that the number of requested threads is the number we put in
	}
//omp_set_dynamic(0);     // Explicitly disable dynamic teams
/* Fork a team of threads giving them their own copies of variables */
	#pragma omp parallel private(nthreads, tid)
	{
	tid = omp_get_thread_num();
	if(tid == 0)
	{
	int N= omp_get_num_threads();
	printf("the total number of threads is %d \n",N);
	}
	#pragma omp for
        for (int i=1; i<50000000; i++){
	//printf("%d\n",i);
        res =  dot_product(a, b);
	//res +=res2 +i;
        }
	}
gettimeofday(&end, NULL);
delta = ((end.tv_sec  - start.tv_sec) * 1000000u +
         end.tv_usec - start.tv_usec) / 1.e6;

printf("the time spent %f\n and result is %f\n",delta,res);
return 0;
}
