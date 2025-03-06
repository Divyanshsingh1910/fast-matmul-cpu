import matplotlib.pyplot as plt

# Data points
matrix_sizes = [32, 64, 128, 256, 512, 1024, 2048, 4096, 8192]

# sal# Data for avx2 (color: orange)
avx2_gflops = [25.068550, 33.931064, 248.418569, 32.895676, 77.654521, 175.522338, 715.264872, 2522.940952, 3956.045887]

# Data for avx512 (color: red)
avx512_gflops = [29.335694, 35.890400, 284.657501, 37.203308, 84.086760, 168.925704, 745.217017, 2836.829772, 5403.050113]

# Dataykova data (all in blue, with increasing brightness)
salykova_56 = [0.943329, 7.686802, 58.532780, 324.508377, 1167.769979, 1929.472776, 2037.473721, 2254.121963, 2162.681566]
salykova_72 = [1.702275, 12.879467, 88.036171, 441.515027, 1334.833258, 1932.906814, 1688.532771, 1861.771941, 1987.063654]
salykova_96 = [1.009233, 9.127924, 65.181029, 370.119396, 1198.091982, 2164.084025, 2053.526334, 2313.428555, 2386.399719]
salykova_112 = [1.270932, 9.682463, 68.132261, 397.903518, 1372.424584, 2393.266867, 2103.862126, 2405.397985, 2387.104259]

# Create a high-definition figure (300 dpi)
plt.figure(figsize=(10, 6), dpi=300)

# Plot the avx2 data (red)
plt.plot(matrix_sizes, avx2_gflops, marker='o', color='orange', label='avx2')
plt.plot(matrix_sizes, avx512_gflops, marker='o', color='red', label='avx512')

# Plot salykova data with blue colors at increasing brightness
# Brightness is modeled by adjusting the blue channel intensity: (R, G, B)
plt.plot(matrix_sizes, salykova_56, marker='o', color='blue', alpha = 0.3, label='salykova_56')
plt.plot(matrix_sizes, salykova_72, marker='o', color='blue', alpha = 0.5, label='salykova_72')
plt.plot(matrix_sizes, salykova_96, marker='o', color='blue', alpha = 0.7, label='salykova_96')
plt.plot(matrix_sizes, salykova_112, marker='o', color='blue', alpha = 0.9, label='salykova_112')

# Labeling the plot
plt.xlabel('Matrix Size')
plt.ylabel('Average GFLOPS')
plt.title('salykova-perf comparsion')
plt.legend()
plt.grid(True)
plt.tight_layout()

plt.show()
