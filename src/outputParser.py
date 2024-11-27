import matplotlib.pyplot as plt
import numpy as np

# Data from the table
bins = np.arange(10, 31)  # Bin edges from 10 to 30
frequencies = [
    10, 15, 13, 4, 7, 11, 8, 9, 8, 7, 6, 5, 9, 1, 8, 10, 1, 8, 6, 11
]  # Frequencies corresponding to each bin

# Create a histogram plot
plt.figure(figsize=(10, 6))
plt.bar(bins[:-1] + 0.5, frequencies, width=1, edgecolor='black', color='skyblue', align='center')

# Add labels and title
plt.xlabel("Value Range", fontsize=12)
plt.ylabel("Frequency", fontsize=12)
plt.title("Histogram of Value Distribution", fontsize=14)
plt.xticks(np.arange(10, 31, 1), rotation=45)  # Tick marks for clarity
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Display the graph
plt.tight_layout()
plt.show()
