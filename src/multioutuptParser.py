import matplotlib.pyplot as plt

# Data values
x = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120]
y = [1914.64, 33830.7, 4231.18, 9568.96, 5695.3, 1283.27, 6288.55, 5991.05, 15692.1, 4965.45, 19432.8, 6536.81]

# Plotting the graph
plt.figure(figsize=(10, 6))
plt.plot(x, y, marker='o', linestyle='-', color='b')

# Labels
plt.xlabel("Doba pozornosti (sekunda)", fontsize=12)
plt.ylabel("Interval mezi reklamami (sekunda)", fontsize=12)
plt.title("Závislost mezi dobou pozornosti a intervalem mezi reklamami", fontsize=14)
plt.grid(True)
plt.tight_layout()

# Display the graph
plt.savefig("attention_time_vs_ad_interval.png")
