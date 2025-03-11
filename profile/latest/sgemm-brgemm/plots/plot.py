#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# Set plot style
sns.set(style="whitegrid")
plt.figure(figsize=(12, 8))

# Read performance data
df = pd.read_csv('perf_logs.txt')

# Convert matrix size to a label for x-axis
matrix_sizes = [int(size) for size in df['MatrixSize1']]
df['Size'] = matrix_sizes

# Create the plot
plt.plot(df['Size'], df['SGEMM_GFLOPS'], 'o-', color='blue', linewidth=2, markersize=8, label='SGEMM')
plt.plot(df['Size'], df['BRGEMM_GFLOPS'], 's-', color='red', linewidth=2, markersize=8, label='BRGEMM')

# Set x-axis to log scale to better visualize the range of matrix sizes
plt.xscale('log', base=2)

# Add labels and title
plt.xlabel('Matrix Size (2*M=2*N=K)', fontsize=14)
plt.ylabel('Performance (GFLOPS)', fontsize=14)
plt.title('SGEMM vs BRGEMM Performance Comparison', fontsize=16)

# Add legend
plt.legend(fontsize=12)

# Add grid
plt.grid(True, which="both", ls="-", alpha=0.2)

# Improve x-axis ticks
plt.xticks(matrix_sizes, labels=[str(size) for size in matrix_sizes], rotation=45)

# Add performance ratio as text annotations
for i, size in enumerate(df['Size']):
    ratio = df['BRGEMM_GFLOPS'].iloc[i] / df['SGEMM_GFLOPS'].iloc[i]
    plt.annotate(f"{ratio:.2f}x", 
                 xy=(size, max(df['SGEMM_GFLOPS'].iloc[i], df['BRGEMM_GFLOPS'].iloc[i]) + 100),
                 ha='center', 
                 fontsize=9)

# Adjust layout
plt.tight_layout()

# Save the figure
plt.savefig('performance_comparison.png', dpi=300)
print("Plot saved as performance_comparison.png")

# Generate a second plot showing relative speedup
plt.figure(figsize=(12, 6))
speedup = df['BRGEMM_GFLOPS'] / df['SGEMM_GFLOPS']
plt.bar(range(len(df)), speedup, color='green', alpha=0.7)
plt.axhline(y=1.0, color='r', linestyle='--')
plt.xticks(range(len(df)), [str(size) for size in df['Size']], rotation=45)
plt.xlabel('Matrix Size (M=N=K)', fontsize=14)
plt.ylabel('Speedup Ratio (BRGEMM/SGEMM)', fontsize=14)
plt.title('BRGEMM Speedup Relative to SGEMM', fontsize=16)
plt.grid(axis='y', alpha=0.3)
plt.tight_layout()
plt.savefig('speedup_ratio.png', dpi=300)
print("Speedup ratio plot saved as speedup_ratio.png")

# Print summary statistics
print("\nPerformance Summary:")
print(f"Average SGEMM performance: {df['SGEMM_GFLOPS'].mean():.2f} GFLOPS")
print(f"Average BRGEMM performance: {df['BRGEMM_GFLOPS'].mean():.2f} GFLOPS")
print(f"Average speedup: {speedup.mean():.2f}x")
print(f"Maximum speedup: {speedup.max():.2f}x at matrix size {df['Size'].iloc[speedup.argmax()]}")
