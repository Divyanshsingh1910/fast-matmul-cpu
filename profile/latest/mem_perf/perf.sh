#!/bin/bash

# Array of matrix sizes to test
sizes=(32 64 128 256 512 1024 2048 4096 8192)

# Print table header
echo "Matrix Size | L1 Miss % | L2 Miss %"
echo "------------|-----------|-----------"

# Loop over each matrix size
for size in "${sizes[@]}"; do
    # Run perf stat and capture output (redirect stderr to stdout)
    perf_output=$(perf stat -e L1-dcache-loads,L1-dcache-load-misses,l2_rqsts.references,l2_rqsts.miss ./benchmark.out "$size" 2>&1)

    # Extract event counts, removing commas for numerical processing
    l1_loads=$(echo "$perf_output" | grep "L1-dcache-loads " | awk '{print $1}' | tr -d ',')
    l1_misses=$(echo "$perf_output" | grep "L1-dcache-load-misses " | awk '{print $1}' | tr -d ',')
    l2_refs=$(echo "$perf_output" | grep "l2_rqsts.references " | awk '{print $1}' | tr -d ',')
    l2_misses=$(echo "$perf_output" | grep "l2_rqsts.miss " | awk '{print $1}' | tr -d ',')

    # Check if all counts were successfully extracted
    if [ -n "$l1_loads" ] && [ -n "$l1_misses" ] && [ -n "$l2_refs" ] && [ -n "$l2_misses" ]; then
        # Calculate percentages using bc for floating-point arithmetic
        l1_miss_pct=$(echo "scale=2; ($l1_misses / $l1_loads) * 100" | bc)
        l2_miss_pct=$(echo "scale=2; ($l2_misses / $l2_refs) * 100" | bc)

        # Print row in table format
        printf "%-11d | %-9s | %-9s\n" "$size" "$l1_miss_pct" "$l2_miss_pct"
    else
        echo "Error: Could not parse perf output for size $size"
    fi
done