
"""
gen_chart_task.py

Generates a Horizontal Bar Chart for Task Stack Pressure.
- Visualizes "Minimum Ever Free Stack" for top tasks.
- Color codes risk: Red (<512B), Orange (<1024B), Green (>1024B).

CONFIGURATION:
- TOP_N: Number of tasks to show (default 20).
- THRESHOLD_CRITICAL: Byte count below which bar turns RED.
- THRESHOLD_WARNING: Byte count below which bar turns ORANGE.

USAGE:
python scripts/gen_chart_task.py --log current_log.txt --output docs/images/resource_report
"""

import matplotlib.pyplot as plt
import argparse
import os
import sys
import numpy as np
from chart_utils import parse_log_data

# ==========================================
# 1. VISUAL CONFIGURATION
# ==========================================
plt.style.use('ggplot')

TOP_N = 20
THRESHOLD_CRITICAL = 512
THRESHOLD_WARNING = 1024

COLORS = {
    'critical': '#c0392b', # Red
    'warning':  '#f39c12', # Orange
    'safe':     '#27ae60'  # Green
}

# ==========================================
# 2. GENERATION LOGIC
# ==========================================

def generate_task_chart(tasks, output_dir):
    if not tasks:
        print("Error: No task data found in log.")
        return

    # Sort by lowest stack free (Risk)
    tasks.sort(key=lambda x: x['stack_free'])
    top_items = tasks[:TOP_N]
    
    names = [t['name'] for t in top_items]
    free_stack = [t['stack_free'] for t in top_items]
    
    fig, ax = plt.subplots(figsize=(10, 8))
    y_pos = np.arange(len(names))
    
    bar_colors = []
    for val in free_stack:
        if val < THRESHOLD_CRITICAL:
            bar_colors.append(COLORS['critical'])
        elif val < THRESHOLD_WARNING:
            bar_colors.append(COLORS['warning'])
        else:
            bar_colors.append(COLORS['safe'])

    # Horizontal Bar
    bars = ax.barh(y_pos, free_stack, align='center', color=bar_colors)
    ax.set_yticks(y_pos)
    ax.set_yticklabels(names)
    ax.invert_yaxis() # Top risk at top
    
    ax.set_xlabel('Minimum Ever Free Stack (Bytes)')
    ax.set_title('Task Stack Pressure (Risk Analysis)')
    
    # Value Labels
    ax.bar_label(bars, fmt='%d', padding=3)
    
    # Watermark
    plt.axvline(x=THRESHOLD_CRITICAL, color='red', linestyle='--', alpha=0.5, label=f'Critical ({THRESHOLD_CRITICAL}B)')
    
    out_path = os.path.join(output_dir, 'task_pressure_bar.png')
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"Generated Task Chart: {out_path}")

# ==========================================
# 3. MAIN ENTRY POINT
# ==========================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate Task Pressure Chart')
    parser.add_argument('--log', type=str, required=True, help='Path to log file')
    parser.add_argument('--output', type=str, default='.', help='Output directory')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.output):
        os.makedirs(args.output)
        
    with open(args.log, 'r') as f:
        log_content = f.read()
    
    data = parse_log_data(log_content)
    generate_task_chart(data['tasks'], args.output)
