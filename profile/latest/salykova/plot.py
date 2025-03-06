import re
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

def parse_logs(log_file):
    """Parses the perf_logs.txt file and returns a Pandas DataFrame."""

    data = []
    current_combination = {}

    with open(log_file, 'r') as f:
        for line in f:
            line = line.strip()

            # Match combination line
            match = re.match(r"Combination: NTHREADS=(\d+), MR=(\d+), NR=(\d+), MC_factor=(\d+), NC_factor=(\d+)", line)
            if match:
                current_combination = {
                    'NTHREADS': int(match.group(1)),
                    'MR': int(match.group(2)),
                    'NR': int(match.group(3)),
                    'MC_factor': int(match.group(4)),
                    'NC_factor': int(match.group(5))
                }

            # Match GFLOPS data
            match = re.match(r"(\d+), (\d+\.?\d*)", line)
            if match and current_combination:
                matrix_size = int(match.group(1))
                gflops = float(match.group(2))
                data.append({
                    **current_combination,
                    'MatrixSize': matrix_size,
                    'GFLOPS': gflops
                })

    return pd.DataFrame(data)


def plot_performance(df):
    """Generates plots showing the effect of each parameter."""

    matrix_sizes = sorted(df['MatrixSize'].unique())
    params = ['NTHREADS', 'MR', 'NR', 'MC_factor', 'NC_factor']

    for param in params:
        plt.figure(figsize=(12, 6))
        sns.lineplot(x='MatrixSize', y='GFLOPS', hue=param, data=df, marker='o')
        plt.title(f'GFLOPS vs. Matrix Size for different {param} values')
        plt.xlabel('Matrix Size')
        plt.ylabel('GFLOPS')
        plt.xscale('log', base=2)  # Use log scale for matrix size
        plt.xticks(matrix_sizes, matrix_sizes)  # Ensure all matrix sizes are shown
        plt.grid(True)
        plt.legend(title=param)
        plt.tight_layout()
        plt.show()


def analyze_performance(df):
    """Analyzes the parsed data for best/worst combinations and parameter sensitivity."""
    # --- Find best and worst combinations ---
    # Calculate average GFLOPS for each combination, considering different matrix sizes.
    avg_gflops = df.groupby(['NTHREADS', 'MR', 'NR', 'MC_factor', 'NC_factor'])['GFLOPS'].mean().reset_index()
    best_combination = avg_gflops.loc[avg_gflops['GFLOPS'].idxmax()]
    worst_combination = avg_gflops.loc[avg_gflops['GFLOPS'].idxmin()]

    print("Best Combination (Highest Average GFLOPS):")
    print(best_combination)
    print("\nWorst Combination (Lowest Average GFLOPS):")
    print(worst_combination)

    # --- Parameter Sensitivity ---
    # Calculate the range of GFLOPS for each parameter, at each MatrixSize.
    print("\nParameter Sensitivity Analysis:")
    for matrix_size in sorted(df['MatrixSize'].unique()):
      print(f"\nMatrix Size: {matrix_size}")
      matrix_size_df = df[df['MatrixSize'] == matrix_size]

      for param in ['NTHREADS', 'MR', 'NR', 'MC_factor', 'NC_factor']:
            param_sensitivity = matrix_size_df.groupby(param)['GFLOPS'].agg(['min', 'max', 'mean', 'std']).reset_index()
            param_sensitivity['range'] = param_sensitivity['max'] - param_sensitivity['min']
            param_sensitivity = param_sensitivity.sort_values('range', ascending=False)

            print(f"\nSensitivity to {param}:")
            print(param_sensitivity)

def plot_heatmap(df, parameter, matrix_size):
    """Plots a heatmap for a specific parameter and matrix size."""

    # Filter the DataFrame for the selected matrix size
    df_filtered = df[df['MatrixSize'] == matrix_size]

    # Determine the two varying parameters based on the fixed parameter
    all_params = ['NTHREADS', 'MR', 'NR', 'MC_factor', 'NC_factor']
    other_params = [p for p in all_params if p != parameter]

    # Find two parameters with most variance
    variances = []
    for param in other_params:
        variance = df_filtered.groupby(param)['GFLOPS'].var().mean()  #average of variance
        variances.append((variance,param))

    variances.sort(reverse=True) #find the most influecial parameters

    param_x = variances[0][1]
    param_y = variances[1][1]


    # Create a pivot table for the heatmap
    pivot_table = df_filtered.pivot_table(
        values='GFLOPS',
        index=param_y,
        columns=param_x,
        aggfunc=np.mean  # Use mean if multiple values exist for a combination
    )

    # Plot the heatmap
    plt.figure(figsize=(10, 8))
    sns.heatmap(pivot_table, annot=True, fmt=".2f", cmap="viridis")
    plt.title(f'GFLOPS Heatmap for {param_x} vs {param_y} at Matrix Size {matrix_size}')
    plt.xlabel(param_x)
    plt.ylabel(param_y)
    plt.tight_layout()
    plt.show()



def main():
    log_file = 'perf_logs.txt'  # Replace with your actual log file
    df = parse_logs(log_file)

    if df.empty:
        print("Error: No data parsed from the log file.  Check the file format and parsing logic.")
        return

    analyze_performance(df)
    plot_performance(df)
    plot_heatmap(df, 'NTHREADS', 1024)  # Example: heatmap of MR vs. NR for Matrix Size 1024.
    plot_heatmap(df, 'MR', 4096)
    plot_heatmap(df, 'NC_factor', 8192)


if __name__ == "__main__":
    main()