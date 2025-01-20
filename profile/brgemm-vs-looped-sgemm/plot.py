import matplotlib.pyplot as plt
import pandas as pd
import io

# Your CSV data as a string
csv_data = """Testing batch size: 1
method,time_ms,gflops
looped,2.035,4221.01
brgemm,2.093,4104.08
      
Testing batch size: 2
method,time_ms,gflops
looped,6.845,2509.92
brgemm,6.378,2693.42
      
Testing batch size: 4
method,time_ms,gflops
looped,8.575,4007.19
brgemm,5.728,5998.08
      
Testing batch size: 8
method,time_ms,gflops
looped,22.664,3032.12
brgemm,12.452,5518.72
      
Testing batch size: 12
method,time_ms,gflops
looped,34.081,3024.55
brgemm,28.934,3562.62
      
Testing batch size: 16
method,time_ms,gflops
looped,45.538,3018.09
brgemm,30.123,4562.62
      
Testing batch size: 32
method,time_ms,gflops
looped,91.151,3015.62
brgemm,51.325,5355.59
      
Testing batch size: 64
method,time_ms,gflops
looped,188.722,2913.04
brgemm,114.816,4788.13"""

# Process the data
batch_sizes = []
data_frames = []

for block in csv_data.split('\nTesting batch size: ')[1:]:
    lines = block.strip().split('\n')
    batch_size = int(lines[0])
    df = pd.read_csv(io.StringIO('\n'.join(lines[1:])))
    df['batch_size'] = batch_size
    data_frames.append(df)

# Combine all data
final_df = pd.concat(data_frames)

# Create figure with two subplots
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

# Plot execution time
for method in ['looped', 'brgemm']:
    method_data = final_df[final_df['method'] == method]
    ax1.plot(method_data['batch_size'], method_data['time_ms'], 
             marker='o', label=method)

ax1.set_xlabel('Batch Size')
ax1.set_ylabel('Time (ms)')
ax1.set_title('Execution Time vs Batch Size')
ax1.grid(True)
ax1.legend()
# ax1.set_xscale('log', base=2)
# ax1.set_yscale('log', base=2)

# Plot GFLOPS
for method in ['looped', 'brgemm']:
    method_data = final_df[final_df['method'] == method]
    ax2.plot(method_data['batch_size'], method_data['gflops'], 
             marker='o', label=method)

ax2.set_xlabel('Batch Size')
ax2.set_ylabel('GFLOPS')
ax2.set_title('Performance (GFLOPS) vs Batch Size')
ax2.grid(True)
ax2.legend()
# ax2.set_xscale('log', base=2)

# Adjust layout and display
plt.tight_layout()
plt.show()
