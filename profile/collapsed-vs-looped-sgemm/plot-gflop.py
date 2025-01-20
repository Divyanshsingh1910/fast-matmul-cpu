import matplotlib.pyplot as plt

# Define your x and y data for GFLOPS
batch_x               = [1,      2,      4,      8,      12,     16,      32,      64]
looped_gemm_gflops_y  = [3219.85, 5353.51, 5452.40, 5231.43, 5146.27, 5422.67, 5219.30, 4831.70]
collapsed_gemm_gflops_y = [5474.12, 5958.38, 6219.80, 6390.22, 6661.42, 6713.02, 6512.97, 6567.87]

# Create the plot for GFLOPS
plt.plot(batch_x, looped_gemm_gflops_y, marker='o', linestyle='-', label='looped-gemm')
plt.plot(batch_x, collapsed_gemm_gflops_y, marker='o', linestyle='-', label='collapsed-gemm')

# Add labels and title
plt.xlabel('batch size')
plt.ylabel('GFLOPS')
plt.title('GEMM Profiling - GFLOPS')

# Add a legend
plt.legend()

# Show the plot
plt.show()
