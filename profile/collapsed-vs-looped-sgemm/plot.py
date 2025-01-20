import matplotlib.pyplot as plt

# Define your x and y data
batch_x               = [1,      2,      4,      8,      12,     16,      32,      64]
looped_gemm_time_y    = [10.671, 12.836, 25.207, 52.544, 80.120, 101.381, 210.663, 455.125]
collapsed_gemm_time_y = [6.277,  11.533, 22.097, 43.015, 61.896, 81.894,  168.819, 334.815]

# Create the plot
plt.plot(batch_x, looped_gemm_time_y, marker='o', linestyle='-', label='looped-gemm')
plt.plot(batch_x, collapsed_gemm_time_y, marker='o', linestyle='-', label='collapsed-gemm')

# Add labels and title
plt.xlabel('batch size')
plt.ylabel('time(ms)')
plt.title('GEMM Profiling - time')

# Add a legend
plt.legend()

# Show the plot
plt.show()

