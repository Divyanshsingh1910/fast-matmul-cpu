#!/bin/bash

# Array of batch sizes to test
batch_sizes=(1 2 4 8 12 16 32 64)

# Create results directory if it doesn't exist
mkdir -p results

# Create CSV header
echo "batch_size,method,time_ms,gflops" > results/benchmark_results.csv

# Loop through each batch size
for batch in "${batch_sizes[@]}"
do
    echo "Testing batch size: $batch"
    
    # Compile with current batch size
    gcc -O3 -march=native -I${MKLROOT}/include brgemm.c \
        -L${MKLROOT}/lib/intel64 -Wl,--no-as-needed \
        -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl -fopenmp \
        -o brgemm_test -DDIMB=$batch
    
    if [ $? -eq 0 ]; then
        # Run and process output
        ./brgemm_test | tail -n 2 | while read -r line; do
            echo "$batch,$line" >> results/benchmark_results.csv
        done
    else
        echo "Compilation failed for batch size $batch"
    fi
done

echo "Testing complete. Results saved in results/benchmark_results.csv"