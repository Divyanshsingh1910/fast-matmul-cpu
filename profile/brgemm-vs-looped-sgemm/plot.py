import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Read the CSV file
df = pd.read_csv('results.csv')

# Clean and reshape the data
df_clean = pd.DataFrame({
    'batch_size': df['batch_size'].repeat(2),
    'method': ['looped', 'brgemm'] * (len(df) // 2),
    'time_ms': df.iloc[::2]['runs:'].tolist() + df.iloc[1::2]['runs:'].tolist(),
    'gflops': df.iloc[::2]['gflops'].tolist() + df.iloc[1::2]['gflops'].tolist()
})

# Set the style
plt.style.use('seaborn')
sns.set_palette("husl")

# Create figure with two subplots
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

# Plot execution time
sns.lineplot(data=df_clean, x='batch_size', y='time_ms', hue='method', marker='o', ax=ax1)
ax1.set_title('Execution Time vs Batch Size')
ax1.set_xlabel('Batch Size')
ax1.set_ylabel('Time (ms)')
ax1.grid(True)

# Plot GFLOPS
sns.lineplot(data=df_clean, x='batch_size', y='gflops', hue='method', marker='o', ax=ax2)
ax2.set_title('Performance vs Batch Size')
ax2.set_xlabel('Batch Size')
ax2.set_ylabel('GFLOPS')
ax2.grid(True)

# Adjust layout and save
plt.tight_layout()
plt.savefig('benchmark_comparison.png', dpi=300, bbox_inches='tight')
plt.close()

# Print summary statistics
print("\nPerformance Summary:")
print("-" * 50)
summary = df_clean.groupby('method').agg({
    'time_ms': ['mean', 'min', 'max'],
    'gflops': ['mean', 'min', 'max']
}).round(2)
print(summary)

# Calculate average speedup
looped_times = df_clean[df_clean['method'] == 'looped']['time_ms']
brgemm_times = df_clean[df_clean['method'] == 'brgemm']['time_ms']
avg_speedup = (looped_times.mean() / brgemm_times.mean())
print(f"\nAverage BRGEMM speedup: {avg_speedup:.2f}x")
