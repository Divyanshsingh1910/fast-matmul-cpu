#!/bin/bash

# Define the output file
OUTPUT_FILE="batch_perf_logs.csv"

# Clear the output file if it exists
echo "MatrixSize,BatchSize,SGEMM_GFLOPS,BRGEMM_GFLOPS,Speedup" > $OUTPUT_FILE

# Define matrix sizes to test
MATRIX_SIZES=(512 4096 8192)

# Define batch sizes to test
BATCH_SIZES=(2 4 8 16 32 64 128 256)

# Number of iterations for each test
ITERATIONS=5

# Run benchmarks for each matrix size and batch size
for size in "${MATRIX_SIZES[@]}"; do
    for batch in "${BATCH_SIZES[@]}"; do
        echo "Running benchmark for matrix size $size x $size x $size with batch size $batch..."
        
        # Run the benchmark command
        result=$(./benchmark_batch.out $size $size $size $batch $ITERATIONS | grep -v "Matrix Size")
        
        # Extract and format the result
        # The expected format from C code is: [m,n,k], batch_size, sgemm_gflops, brgemm_gflops, speedup
        # We want to convert it to: m,batch_size,sgemm_gflops,brgemm_gflops,speedup
        
        # Extract the values and remove brackets
        formatted_result=$(echo $result | sed 's/\[//g' | sed 's/\]//g' | sed 's/ //g')
        
        # The first part has commas between m,n,k but we only need m since they're all the same
        matrix_size=$(echo $formatted_result | cut -d',' -f1)
        rest_of_result=$(echo $formatted_result | cut -d',' -f4-)
        
        # Append to the output file
        echo "$matrix_size,$rest_of_result" >> $OUTPUT_FILE
        
        echo "Completed benchmark for matrix size $size with batch size $batch"
    done
done

echo "All benchmarks completed. Results saved to $OUTPUT_FILE"

# Optional: Generate quick summary of results
echo ""
echo "Summary of Results:"
echo "==================="
echo ""
echo "Matrix Size: 512"
grep "^512," $OUTPUT_FILE | sort -t',' -k2n

echo ""
echo "Matrix Size: 4096"
grep "^4096," $OUTPUT_FILE | sort -t',' -k2n

echo ""
echo "Matrix Size: 8192"
grep "^8192," $OUTPUT_FILE | sort -t',' -k2n