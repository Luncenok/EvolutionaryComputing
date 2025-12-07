import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Set style
sns.set_style("whitegrid")
plt.rcParams['figure.figsize'] = (10, 6)

# Define instances and configurations
instances = ['TSPA', 'TSPB']
measures = ['edges', 'nodes']
versions = [
    ('avg_all', 'Average Similarity to All Other Local Optima'),
    ('best_1000', 'Similarity to Best of 1000 Local Optima'),
    ('best_method', 'Similarity to Best Solution (ILS)')
]

output_dir = 'output'
charts_dir = 'assignment8/charts'
os.makedirs(charts_dir, exist_ok=True)

def create_chart(instance, measure, version_key, version_title):
    """Create a scatter plot for fitness-similarity correlation"""
    filename = f"{output_dir}/{instance}_{measure}_{version_key}.csv"
    
    if not os.path.exists(filename):
        print(f"Warning: {filename} not found, skipping...")
        return
    
    # Read data
    df = pd.read_csv(filename)
    
    # Extract correlation (all rows have the same correlation value)
    correlation = df['correlation'].iloc[0]
    
    # Create scatter plot
    fig, ax = plt.subplots(figsize=(10, 6))
    
    scatter = ax.scatter(df['objective'], df['similarity'], alpha=0.5, s=20)
    
    # Add regression line
    z = np.polyfit(df['objective'], df['similarity'], 1)
    p = np.poly1d(z)
    x_line = np.linspace(df['objective'].min(), df['objective'].max(), 100)
    ax.plot(x_line, p(x_line), "r--", alpha=0.8, linewidth=2, label=f'Correlation: {correlation:.4f}')
    
    # Labels and title
    measure_name = 'Common Edges' if measure == 'edges' else 'Common Nodes'
    ax.set_xlabel('Objective Function Value', fontsize=12)
    ax.set_ylabel(f'Similarity ({measure_name})', fontsize=12)
    ax.set_title(f'{instance} - {version_title}\n{measure_name}', fontsize=14, fontweight='bold')
    
    ax.legend(fontsize=11, loc='best')
    ax.grid(True, alpha=0.3)
    
    # Save chart
    output_filename = f"{charts_dir}/{instance}_{measure}_{version_key}.png"
    plt.tight_layout()
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"Created: {output_filename}")

# Import numpy for regression line
import numpy as np

print("Generating global convexity charts...")
print(f"Reading data from: {output_dir}/")
print(f"Saving charts to: {charts_dir}/\n")

chart_count = 0
for instance in instances:
    for measure in measures:
        for version_key, version_title in versions:
            create_chart(instance, measure, version_key, version_title)
            chart_count += 1

print(f"\nTotal charts created: {chart_count}/12")
print("\nSummary of charts:")
print("=" * 60)

# Print summary table
for instance in instances:
    print(f"\n{instance}:")
    for measure in measures:
        measure_name = 'Common Edges' if measure == 'edges' else 'Common Nodes'
        print(f"  {measure_name}:")
        for version_key, version_title in versions:
            filename = f"{output_dir}/{instance}_{measure}_{version_key}.csv"
            if os.path.exists(filename):
                df = pd.read_csv(filename)
                correlation = df['correlation'].iloc[0]
                print(f"    - {version_title}: r = {correlation:.4f}")

print("\n" + "=" * 60)
print("All charts have been generated successfully!")
