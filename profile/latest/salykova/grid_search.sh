#!/bin/bash

# Truncate the log file to start fresh
> perf_logs.txt

# Define parameter arrays
NTHREADS=(32 56 72 112)
MR=(8 16 24)
NR=(6 8 12 14)
MC_factor=(2 4 6)
NC_factor=(71 94)

# Check if source files exist
if [ ! -f bench.c ] || [ ! -f matmul.c ] || [ ! -f kernel.c ]; then
    echo "Error: One or more source files (bench.c, matmul.c, kernel.c) are missing."
    exit 1
fi

# Nested loops over all parameter combinations
for nthreads in "${NTHREADS[@]}"; do
    for mr in "${MR[@]}"; do
        for nr in "${NR[@]}"; do
            for mc_factor in "${MC_factor[@]}"; do
                for nc_factor in "${NC_factor[@]}"; do
                    # Display progress
                    echo "Processing combination: NTHREADS=$nthreads, MR=$mr, NR=$nr, MC_factor=$mc_factor, NC_factor=$nc_factor"

                    # Compile with current parameters
                    gcc -O3 -march=native -fopenmp bench.c matmul.c kernel.c -o salykova \
                        -DNTHREADS="$nthreads" -DMR="$mr" -DNR="$nr" \
                        -DMC_factor="$mc_factor" -DNC_factor="$nc_factor"
                    if [ $? -ne 0 ]; then
                        echo "Compilation failed for this combination. Skipping."
                        continue
                    fi

                    # Run the benchmark twice
                    ./salykova > run.txt

                    echo "-----" >> perf_logs.txt
                    echo "Combination: NTHREADS=$nthreads, MR=$mr, NR=$nr, MC_factor=$mc_factor, NC_factor=$nc_factor" >> perf_logs.txt
                    cat run.txt >> perf_logs.txt
                done
            done
        done
    done
done

echo "Benchmarking completed."