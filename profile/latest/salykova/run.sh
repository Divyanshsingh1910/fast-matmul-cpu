#!/bin/bash

# Number of runs
runs=3

declare -A sum_gflops

echo "MatrixSize, Average_GFLOPS"

# Run the command three times and collect results
for ((i=1; i<=runs; i++)); do
    while IFS=',' read -r size gflops; do
        if [[ $size != "MatrixSize" ]]; then  # Skip header
            sum_gflops[$size]=$(echo "${sum_gflops[$size]:-0} + $gflops" | bc)
        fi
    done < <(./salykova)
done

# Compute and display averages
for size in "${!sum_gflops[@]}"; do
    avg=$(echo "scale=6; ${sum_gflops[$size]} / $runs" | bc)
    echo "$size, $avg"
done | sort -n

