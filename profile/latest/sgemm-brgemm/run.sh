#!/bin/bash

# Define the output file
OUTPUT_FILE="perf_logs.txt"

# Clear the output file if it exists
echo "MatrixSize,SGEMM_GFLOPS,BRGEMM_GFLOPS" > $OUTPUT_FILE

# Define matrix sizes to test
MATRIX_SIZES=(8 16 32 64 256 512 1024 2048 3072 4096 5120 6144 7168 8192)

# Number of iterations for each test
ITERATIONS=10

# Run benchmarks for each matrix size
for size in "${MATRIX_SIZES[@]}"; do
    echo "Running benchmark for matrix size $size x $size x $size..."
    
    # Run the benchmark command
    result=$(./benchmark.out $size $size $size $ITERATIONS | grep -v "MatrixSize")
    
    # Extract the values and remove brackets and spaces
    formatted_result=$(echo $result | sed 's/\[//g' | sed 's/\]//g' | sed 's/ //g')
    
    # Append to the output file
    echo "$formatted_result" >> $OUTPUT_FILE
    
    echo "Completed benchmark for size $size"
done

echo "All benchmarks completed. Results saved to $OUTPUT_FILE"