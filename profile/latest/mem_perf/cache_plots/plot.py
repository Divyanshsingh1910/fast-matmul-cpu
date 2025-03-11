import pandas as pd
import matplotlib.pyplot as plt

# Define the log files and their corresponding method names
log_files = {
    'AVX2': 'logs/cache_logs_avx2.txt',
    'AVX512': 'logs/cache_logs_avx512.txt',
    'Default': 'logs/cache_logs_default.txt',
    'Salykova': 'logs/cache_logs_salykova.txt'
}

# Read each log file into a DataFrame
data = {}
for method, file in log_files.items():
    data[method] = pd.read_csv(file)

# Define the cache levels and their corresponding metrics
cache_levels = [
    ('L1', 'L1_Loads', 'L1_MissRate'),
    ('L2', 'L2_Loads', 'L2_MissRate'),
    ('LLC', 'LLC_Loads', 'LLC_MissRate')
]

# Define the methods
methods = ['AVX2', 'AVX512', 'Default', 'Salykova']

# Colors for each method (for consistency across plots)
colors = {
    'AVX2': 'blue',
    'AVX512': 'green',
    'Default': 'red',
    'Salykova': 'purple'
}

# Create three separate plots, one for each cache level
for level, loads_metric, missrate_metric in cache_levels:
    # Create a new figure for the plot
    plt.figure(figsize=(10, 6))
    
    # Create twin axes for loads (left) and miss rate (right)
    ax1 = plt.gca()  # Primary axis (left) for loads
    ax2 = ax1.twinx()  # Secondary axis (right) for miss rate
    
    # Plot loads and miss rates for each method
    for method in methods:
        # Plot loads on the left axis (ax1)
        ax1.plot(data[method]['MatrixSize'], data[method][loads_metric], 
                 label=f'{method} Loads', color=colors[method], marker='o', linestyle='-')
        
        # Plot miss rate on the right axis (ax2)
        ax2.plot(data[method]['MatrixSize'], data[method][missrate_metric], 
                 label=f'{method} Miss Rate', color=colors[method], marker='x', linestyle='--')
    
    # Configure the left axis (loads)
    ax1.set_xlabel('Matrix Size')
    ax1.set_ylabel(f'{level} Loads', color='black')
    ax1.set_yscale('log')  # Log scale for loads due to large range
    ax1.tick_params(axis='y', labelcolor='black')
    ax1.grid(True, which="both", ls="--", alpha=0.2)
    
    # Configure the right axis (miss rate)
    ax2.set_ylabel(f'{level} Miss Rate (%)', color='black')
    # Linear scale for miss rate (percentages)
    ax2.tick_params(axis='y', labelcolor='black')
    
    # Set title
    plt.title(f'{level} Loads and Miss Rate vs Matrix Size')
    
    # Combine legends from both axes
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2, loc='upper left')
    
    # Adjust layout to prevent overlap
    plt.tight_layout()
    
    # Save the plot
    plt.show()
    filename = f'{level}_Loads_and_MissRate.png'
    plt.savefig(filename)
    plt.close()  # Close the figure to free memory

print("Three plots have been generated and saved as PNG files: L1_Loads_and_MissRate.png, L2_Loads_and_MissRate.png, LLC_Loads_and_MissRate.png")