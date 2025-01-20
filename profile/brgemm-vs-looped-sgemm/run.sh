#!/bin/bash

# Array of batch sizes to test
batch_sizes=(1 2 4 8 12 16 32 64)

# Create results directory if it doesn't exist
mkdir -p results

# Create or clear the CSV file for results
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
    
    # Check if compilation was successful
    if [ $? -eq 0 ]; then
        # Run the test and capture output
        output=$(./brgemm_test)
        
        # Extract average times and GFLOPS for both methods using awk
        looped_time=$(echo "$output" | grep "Looped SGEMM average" | awk '{print $6}')
        looped_gflops=$(echo "$output" | grep "Looped SGEMM average" | awk -F'(' '{print $2}' | awk '{print $1}')
        brgemm_time=$(echo "$output" | grep "BRGEMM average" | awk '{print $6}')
        brgemm_gflops=$(echo "$output" | grep "BRGEMM average" | awk -F'(' '{print $2}' | awk '{print $1}')
        
        # Save to CSV
        echo "$batch,looped,$looped_time,$looped_gflops" >> results/benchmark_results.csv
        echo "$batch,brgemm,$brgemm_time,$brgemm_gflops" >> results/benchmark_results.csv
        
        echo "  Looped SGEMM: $looped_time ms ($looped_gflops GFLOPS)"
        echo "  BRGEMM: $brgemm_time ms ($brgemm_gflops GFLOPS)"
    else
        echo "Compilation failed for batch size $batch"
    fi
done

echo "Testing complete. Results saved in results/benchmark_results.csv"
