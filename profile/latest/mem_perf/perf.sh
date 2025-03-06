#!/bin/bash

# Array of matrix sizes to test
matrix_sizes=(8 16 32 64 256 512 1024 2048 3072 4096 5120 6144 7168 8192)

# Output file
output_file="cache_logs.txt"

# Write the CSV header
echo "MatrixSize,L1_Loads,L1_Misses,L1_MissRate,L2_Loads,L2_Misses,L2_MissRate,LLC_Loads,LLC_Misses,LLC_MissRate" > "$output_file"

# Loop through each matrix size
for size in "${matrix_sizes[@]}"; do
    echo "Running benchmark for size: $size"

    # Run the perf command and capture the output
    perf_output=$(perf stat -e L1-dcache-loads,L1-dcache-load-misses,l2_rqsts.demand_data_rd_hit,l2_rqsts.demand_data_rd_miss,LLC-loads,LLC-loads-misses ./salykova "$size" 2>&1)
    
    # check if benchmark.out compiled correctly
    if [ $? -ne 0 ]; then
    	echo "Error: benchmark.out execution failed."
    	exit 1
    fi

    # Extract the values using line numbers (as requested, no pattern matching)
    l1_loads=$(echo "$perf_output" | head -n 6 | tail -n 1 | awk '{print $1}')
    l1_misses=$(echo "$perf_output" | head -n 7 | tail -n 1 | awk '{print $1}')
    l2_hits=$(echo "$perf_output" | head -n 8 | tail -n 1 | awk '{print $1}')
    l2_misses=$(echo "$perf_output" | head -n 9 | tail -n 1 | awk '{print $1}')
    llc_loads=$(echo "$perf_output" | head -n 10 | tail -n 1 | awk '{print $1}')
    llc_misses=$(echo "$perf_output" | head -n 11 | tail -n 1 | awk '{print $1}')


    # remove commas from all of the above variables for calculation and csv
    l1_loads=$(echo "$l1_loads" | tr -d ',')
    l1_misses=$(echo "$l1_misses" | tr -d ',')
    l2_hits=$(echo "$l2_hits" | tr -d ',')
    l2_misses=$(echo "$l2_misses" | tr -d ',')
    llc_loads=$(echo "$llc_loads" | tr -d ',')
    llc_misses=$(echo "$llc_misses" | tr -d ',')

    # Calculate miss rates (handle potential division by zero)
    l1_miss_rate=$(awk "BEGIN {if ($l1_loads > 0) printf \"%.2f\", ($l1_misses / $l1_loads) * 100; else printf \"0.00\"}")
    l2_loads=$((l2_hits + l2_misses))
    l2_miss_rate=$(awk "BEGIN {if ($l2_loads > 0) printf \"%.2f\", ($l2_misses / $l2_loads) * 100; else printf \"0.00\"}")
    llc_miss_rate=$(awk "BEGIN {if ($llc_loads > 0) printf \"%.2f\", ($llc_misses / $llc_loads) * 100; else printf \"0.00\"}")

    # Append the results to the CSV file
    echo "$size,$l1_loads,$l1_misses,$l1_miss_rate,$l2_loads,$l2_misses,$l2_miss_rate,$llc_loads,$llc_misses,$llc_miss_rate" >> "$output_file"

done

echo "Performance data saved to: $output_file"