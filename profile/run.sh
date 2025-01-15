#!/bin/bash

# Array of batch sizes to test
batch_sizes=(1 2 4 8 12 16 32 64)

# Create or clear the log file
echo "BRGEMM Performance Tests" > log.txt
echo "======================" >> log.txt
date >> log.txt
echo "" >> log.txt

# Loop through each batch size
for batch in "${batch_sizes[@]}"
do
    echo "Testing batch size: $batch"
    echo "Batch Size: $batch" >> log.txt
    echo "-------------" >> log.txt
    
    # Compile with current batch size
    gcc -O3 -march=native \
        -I${MKLROOT}/include \
        -DBATCH=$batch \
        looped-sgemm.c \
        -L${MKLROOT}/lib/intel64 \
        -Wl,--no-as-needed -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl -fopenmp \
        -o matmul_test
    
    # Check if compilation was successful
    if [ $? -eq 0 ]; then
        # Run and save output to log
        ./matmul_test >> log.txt 2>&1
        echo "" >> log.txt
    else
        echo "Compilation failed for batch size $batch" >> log.txt
        echo "" >> log.txt
    fi
done

echo "Testing complete. Results saved in log.txt"
cat log.txt
