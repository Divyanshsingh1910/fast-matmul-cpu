
> [!important]  I found the Llama.cpp/Llamafile guy's matmul to be very interesting because of the following reasons
> 1. The code is contribude to Llama.cpp by an americal swe who leads the mozilla's llamafile project
> 2. She has a nice blog explaining all the work -> https://justine.lol/matmul/
> 3. In the blog she claims the computation to 2x faster than MKL-DNN in cases when matrix fits in L2. For example, Mistral 7B at fp16 and ~200 prompt token goes from 13 tok/sec to 52 tok/sec after her changes -> https://github.com/ggerganov/llama.cpp/pull/6414
> 4. More importantly, even though the code is 3-4k lines, it is at least 20x more readable than MKL-DNN's code.

#### Llama.cpp's take on matmul
> llama.cpp had the important insight that less is more when it comes to linear algebra. The alpha and beta parameters are never used, so they're always set to to 1 and 0. The op graph for LLMs are designed in such a way that the A matrix is almost always transposed and B is almost never transposed, which means inner dimension dot product can vectorize over contiguous memory. The m/k dimensions are usually evenly divisible by 64. While generating tokens, n=1 is usually the case, which makes matmul a de facto matvec for the performance most people care about. BLAS libraries usually hurt more than they help for matrix-vector multiplication, because it's so computationally simple by comparison. Sort of like the difference between downloading a movie and pinging a server. Matrix vector multiplication is an operation where latency (not throughput) is the bottleneck, and the bloat of fancy libraries has a measurable impact. So llama.cpp does something like this, which goes 233 gigaflops.

```cpp
template <typename T>
void LLMM(int m, int n, int k,
          const T *A, int lda,
          const T *B, int ldb,
          T *C, int ldc) {
#pragma omp parallel for collapse(2) if (m * n * k > 300000)
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j) {
            T d = 0;
            for (int l = 0; l < k; ++l)
                d += A[lda * i + l] * B[ldb * j + l];
            C[ldc * j + i] = d;
        }
}
```

