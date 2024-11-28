import os
import re
import matplotlib.pyplot as plt


def parse_histogram_section(lines, start_index, is_day_histogram):
    """
    Parse histogram section starting at a given index in lines.
    Returns parsed histogram data and the index of the next section.
    """
    bins = []
    frequencies = []
    rel_frequencies = []

    # Skip header lines until reaching the data section
    index = start_index
    while index < len(lines) and "+------------+------------+----------+----------+----------+" not in lines[index]:
        index += 1

    index += 1  # Skip the header line after finding the histogram table

    while index < len(lines) and "|" in lines[index]:
        parts = lines[index].split('|')
        try:
            bin_start = float(parts[1].strip())
            bin_end = float(parts[2].strip())
            frequency = int(parts[3].strip())
            rel_frequency = float(parts[4].strip())

            # For "day" histograms, use only the upper bound ("to") of the bin
            if is_day_histogram:
                bins.append(f"{bin_end}")
            else:
                bins.append(f"{bin_start}-{bin_end}")
            
            frequencies.append(frequency)
            rel_frequencies.append(rel_frequency)
        except ValueError:
            break  # Stop parsing if we encounter invalid data
        index += 1

    return bins, frequencies, rel_frequencies, index


def plot_histogram(bins, frequencies, output_path, title, xlabel, ylabel):
    """
    Create and save a histogram graph using matplotlib with dynamic width
    and average value displayed.
    """
    # Calculate width dynamically: base width + extra width for larger datasets
    base_width = 8
    extra_width_per_bin = 0.2
    dynamic_width = base_width + len(bins) * extra_width_per_bin

    # Calculate the average frequency
    average_value = sum(frequencies) / len(frequencies) if frequencies else 0

    # calculate median value
    median_value = frequencies[len(frequencies) // 2]

    plt.figure(figsize=(dynamic_width, 6))
    plt.bar(bins, frequencies, color='skyblue', alpha=0.7)

    # Initialize a list to store the labels for the legend
    legend_labels = []

    

    if "adds seen per day" in title.lower():
        plt.axhline(y=5, color='red', linestyle='--', label='Max 5 Adds per Day')
        legend_labels.append('Max 5 Adds per Day')
        plt.axhline(y=median_value, color='green', linestyle='--', label=f'Median: {median_value:.2f}')
        legend_labels.append(f'Median: {median_value:.2f}')
    elif "length" not in title.lower():
        plt.axhline(y=average_value, color='orange', linestyle='--', label=f'Average: {average_value:.2f}')
        legend_labels.append(f'Average: {average_value:.2f}')


    # If no lines are added, add a default label
    if not legend_labels:
        legend_labels.append('No data lines')

    # Add title, labels, and legend
    plt.title(title, fontsize=16)
    plt.xlabel(xlabel, fontsize=12)
    plt.ylabel(ylabel, fontsize=12)
    plt.xticks(rotation=45, ha='right')

    # Display legend only if there are labels
    if legend_labels:
        plt.legend(legend_labels, fontsize=12)

    # Adjust layout and save the plot
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()





def extract_and_plot_histograms(file_path):
    """
    Extract all histograms from a raw.out file and generate plots for each.
    """
    with open(file_path, 'r') as file:
        lines = file.readlines()

    index = 0
    histogram_count = 0
    while index < len(lines):
        if "HISTOGRAM" in lines[index]:
            # Extract histogram title
            title_line = lines[index].strip()
            title = title_line.replace("|", "").replace("HISTOGRAM", "").strip()

            # Check if the histogram is about "day"
            is_day_histogram = "day" in title.lower() or "hour" in title.lower()

            # Parse histogram data
            bins, frequencies, _, next_index = parse_histogram_section(lines, index, is_day_histogram)

            if bins and frequencies:
                # Save the plot
                output_dir = os.path.dirname(file_path)
                output_file = os.path.join(output_dir, f"{title.replace(' ', '_')}.png")

                xLabel = "Bins"
                yLabel = "Frequency"
                if is_day_histogram:
                    if "hour" in title.lower():
                        xLabel = "Hour"
                    else:
                        xLabel = "Day"
                    if "more" in title.lower():
                        yLabel = "Number of Occurrences"
                    else:
                        if "adds" in title.lower():
                            yLabel = "Number of Adds"
                        else:
                            yLabel = "Number of Posts"
                if "length" in title.lower():
                    xLabel = "Length (seconds)"
                    if "adds" in title.lower():
                        yLabel = "Number of Adds"
                    else:
                        yLabel = "Number of Posts"
                

                plot_histogram(
                    bins,
                    frequencies,
                    output_file,
                    title=title,
                    xlabel=xLabel,
                    ylabel=yLabel
                )
                print(f"Histogram saved: {output_file}")

                histogram_count += 1

            index = next_index  # Move to the next section
        else:
            index += 1


def process_all_raw_out_files(root_folder):
    """
    Process all raw.out files in the directory structure.
    """
    for root, _, files in os.walk(root_folder):
        for file in files:
            if file == "raw.out":
                file_path = os.path.join(root, file)
                print(f"Processing: {file_path}")
                extract_and_plot_histograms(file_path)


if __name__ == "__main__":
    # Change this path to the root folder you want to process
    root_folder = "tests"
    process_all_raw_out_files(root_folder)
