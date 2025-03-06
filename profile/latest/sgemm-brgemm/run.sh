#!/bin/bash

# Define the output file
OUTPUT_FILE="perf_logs.txt"

# Clear the output file if it exists
echo "MatrixSize1,MatrixSize2,MatrixSize3,SGEMM_GFLOPS,BRGEMM_GFLOPS" > $OUTPUT_FILE

# Define matrix sizes to test for M and N
MATRIX_SIZES=(8 16 32 64 256 512 1024 2048 3072 4096 5120 6144 7168 8192)

# Number of iterations for each test
ITERATIONS=10

# Then run rectangular matrix benchmarks (K = M/2)
echo "Running rectangular matrix benchmarks (K = M/2)..."
for size in "${MATRIX_SIZES[@]}"; do
    # Calculate K as M/2
    k=$((size * 2))
    
    echo "Running benchmark for rectangular matrix size $size x $size x $k..."
    
    # Run the benchmark command
    result=$(./benchmark.out $size $size $k $ITERATIONS | grep -v "MatrixSize")
    
    # Extract the values and remove brackets and spaces
    formatted_result=$(echo $result | sed 's/\[//g' | sed 's/\]//g' | sed 's/ //g')
    
    # Append to the output file
    echo "$size,$size,$k,${formatted_result#*,}" >> $OUTPUT_FILE
    
    echo "Completed benchmark for rectangular size $size x $size x $k"
done

echo "All benchmarks completed. Results saved to $OUTPUT_FILE"