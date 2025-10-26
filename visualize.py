#!/usr/bin/env python3
import matplotlib.pyplot as plt
import os
import numpy as np
import re

def read_csv(filename):
    """Read CSV file and return lists of x, y, costs"""
    x_coords = []
    y_coords = []
    costs = []
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split(';')
            if len(parts) == 3:
                x_coords.append(int(parts[0]))
                y_coords.append(int(parts[1]))
                costs.append(int(parts[2]))
    
    return x_coords, y_coords, costs

def parse_output_file(filename):
    """Parse output.txt to extract all solutions"""
    solutions = {}
    current_instance = None
    current_method = None
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            
            # Match instance name
            instance_match = re.match(r'=-=-= (\w+)\.csv =-=-=', line)
            if instance_match:
                current_instance = instance_match.group(1)
                solutions[current_instance] = {}
                continue
            
            # Match method name
            if line.endswith(':') and current_instance:
                current_method = line[:-1]
                solutions[current_instance][current_method] = {}
                continue
            
            # Match Min value
            if line.startswith('Min:') and current_method:
                min_match = re.search(r'Min: (\d+)', line)
                if min_match:
                    solutions[current_instance][current_method]['objective'] = int(min_match.group(1))
                continue
            
            # Match Best solution
            if line.startswith('Best:') and current_method:
                solution_str = line.replace('Best:', '').strip()
                solution = [int(x) for x in solution_str.split()]
                solutions[current_instance][current_method]['solution'] = solution
                continue
    
    return solutions

def visualize_solution(x_coords, y_coords, costs, solution, instance_name, method_name, objective):
    """Create 2D visualization of the solution"""
    fig, ax = plt.subplots(figsize=(12, 10))
    
    # Plot all nodes colored by cost (intensity visible for all)
    all_nodes = range(len(x_coords))
    vmin = min(costs)
    vmax = max(costs)

    scatter_all = ax.scatter([x_coords[i] for i in all_nodes],
                             [y_coords[i] for i in all_nodes],
                             c=[costs[i] for i in all_nodes],
                             s=30,
                             cmap='YlOrRd',
                             vmin=vmin, vmax=vmax,
                             edgecolors='none',
                             alpha=0.6,
                             zorder=1)

    # Highlight selected nodes (same colormap, larger markers and outline)
    selected_x = [x_coords[i] for i in solution]
    selected_y = [y_coords[i] for i in solution]
    selected_costs = [costs[i] for i in solution]

    scatter_sel = ax.scatter(selected_x, selected_y,
                             c=selected_costs,
                             s=100,
                             cmap='YlOrRd',
                             vmin=vmin, vmax=vmax,
                             edgecolors='black',
                             linewidth=0.5,
                             alpha=0.9,
                             zorder=3)

    # Add colorbar for node costs
    cbar = plt.colorbar(scatter_all, ax=ax)
    cbar.set_label('Node Cost', rotation=270, labelpad=20)
    
    # Draw the Hamiltonian cycle
    for i in range(len(solution)):
        start = solution[i]
        end = solution[(i + 1) % len(solution)]
        ax.plot([x_coords[start], x_coords[end]], 
                [y_coords[start], y_coords[end]], 
                'b-', alpha=0.4, linewidth=1.5, zorder=2)
    
    # Add node indices for selected nodes
    for i in solution:
        ax.annotate(str(i), (x_coords[i], y_coords[i]), 
                   fontsize=6, ha='center', va='center',
                   bbox=dict(boxstyle='round,pad=0.2', facecolor='white', 
                            edgecolor='none', alpha=0.7), zorder=4)
    
    ax.set_xlabel('X Coordinate', fontsize=12)
    ax.set_ylabel('Y Coordinate', fontsize=12)
    ax.set_title(f'{instance_name} - {method_name}\nObjective: {objective} | Selected: {len(solution)} nodes', 
                fontsize=14, fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.set_aspect('equal', adjustable='box')
    
    plt.tight_layout()
    os.makedirs('output', exist_ok=True)
    filename = f'output/{instance_name}_{method_name.replace(" ", "_").replace("(", "").replace(")", "")}.png'
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f'Saved: {filename}')
    plt.close()

# Parse solutions from output.txt
print('Parsing output.txt...')
solutions = parse_output_file('output.txt')

# Generate all visualizations
for instance in solutions.keys():
    print(f'\nProcessing {instance}.csv...')
    x_coords, y_coords, costs = read_csv(f'input/{instance}.csv')
    
    for method, data in solutions[instance].items():
        if 'solution' in data and 'objective' in data:
            visualize_solution(x_coords, y_coords, costs, 
                             data['solution'], instance, method, data['objective'])

print('\nAll visualizations generated!')
