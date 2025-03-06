#!/bin/bash

# Set up Intel oneAPI environment
source /opt/intel/oneapi/setvars.sh

# Matrix sizes to profile
sizes=(32 64 128 256 512 1024 2048 4096 8192)

# Arrays to store the metrics
L1_miss_rates=()
L2_miss_rates=()
memory_bandwidths=()
avx2_instructions=()
avx512_instructions=()
amx_instructions=()

for size in "${sizes[@]}"; do
    echo "Profiling matrix size: $size with VTune"

    # Memory Access analysis for cache misses and bandwidth
    vtune -collect memory-access -result-dir vtune_memory_$size -- ./benchmark.out $size
    vtune -report metric -format csv -result-dir vtune_memory_$size > vtune_memory_$size.csv

    # Microarchitecture Exploration for vector instruction usage
    vtune -collect uarch-exploration -result-dir vtune_uarch_$size -- ./benchmark.out $size
    vtune -report metric -format csv -result-dir vtune_uarch_$size > vtune_uarch_$size.csv

    # Parse memory-access CSV for L1 and L2 miss rates and memory bandwidth
    L1_miss_rate=$(awk -F, '/L1D_Load_Miss_Rate/{print $2}' vtune_memory_$size.csv)
    L2_miss_rate=$(awk -F, '/L2_Load_Miss_Rate/{print $2}' vtune_memory_$size.csv)
    memory_bandwidth_bytes_per_second=$(awk -F, '/Dram_BW_Bytes_Per_Second/{print $2}' vtune_memory_$size.csv)
    memory_bandwidth_gbs=$(bc -l <<< "$memory_bandwidth_bytes_per_second / 1000000000")

    # Parse uarch-exploration CSV for vector instructions
    avx2_instructions_count=$(awk -F, '/Vector_256b_Instructions_Retired/{print $2}' vtune_uarch_$size.csv)
    avx512_instructions_count=$(awk -F, '/Vector_512b_Instructions_Retired/{print $2}' vtune_uarch_$size.csv)

    # Parse uarch-exploration CSV for AMX instructions
    amx_config_inst=$(awk -F, '/Tile_Configuration_Instructions_Retired/{print $2}' vtune_uarch_$size.csv)
    amx_load_inst=$(awk -F, '/Tile_Load_Instructions_Retired/{print $2}' vtune_uarch_$size.csv)
    amx_store_inst=$(awk -F, '/Tile_Store_Instructions_Retired/{print $2}' vtune_uarch_$size.csv)
    amx_arith_inst=$(awk -F, '/Tile_Arithmetic_Instructions_Retired/{print $2}' vtune_uarch_$size.csv)
    total_amx_instructions=$((amx_config_inst + amx_load_inst + amx_store_inst + amx_arith_inst))

    # Store the metrics
    L1_miss_rates+=("$L1_miss_rate")
    L2_miss_rates+=("$L2_miss_rate")
    memory_bandwidths+=("$memory_bandwidth_gbs")
    avx2_instructions+=("$avx2_instructions_count")
    avx512_instructions+=("$avx512_instructions_count")
    amx_instructions+=("$total_amx_instructions")
done

# Print the table
echo "Matrix Size   L1 Miss %   L2 Miss %   Memory BW (GB/s)   AVX2 Instructions   AVX512 Instructions   AMX Instructions"
for i in "${!sizes[@]}"; do
    printf "%-10d %-10.2f %-10.2f %-15.2f %-15d %-15d %-15d\n" "${sizes[$i]}" "${L1_miss_rates[$i]}" "${L2_miss_rates[$i]}" "${memory_bandwidths[$i]}" "${avx2_instructions[$i]}" "${avx512_instructions[$i]}" "${amx_instructions[$i]}"
done