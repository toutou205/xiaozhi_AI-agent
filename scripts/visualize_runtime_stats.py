import re
import matplotlib.pyplot as plt
import sys
import os
import matplotlib.patches as mpatches

# Use ggplot style for a clean look
plt.style.use('ggplot')

def parse_runtime_stats(filepath):
    tasks = []
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Try to find the RUN_TIME_STATS block
    match = re.search(r'RUN_TIME_STATS:(.*?)--- END RESOURCE LOG ---', content, re.DOTALL)
    if match:
        data_block = match.group(1)
    else:
        # Fallback: assume the whole file is the block if header not found
        data_block = content
        
    lines = data_block.strip().split('\n')
    
    # Regex to capture: Name (anything) + Space + AbsTime (digits) + Space + Percentage (digits or <1)%
    pattern = re.compile(r'^\s*(\S+)\s+(\d+)\s+([<>\d]+)%?')
    
    total_time = 0
    parsed_tasks = []
    
    for line in lines:
        line = line.strip()
        if not line or line.startswith('Task Name') or line.startswith('---'):
            continue
            
        m = pattern.match(line)
        if m:
            name = m.group(1)
            abs_time = int(m.group(2))
            parsed_tasks.append({'name': name, 'time': abs_time})
            total_time += abs_time
            
    # Sort by time desc
    parsed_tasks.sort(key=lambda x: x['time'], reverse=True)
    
    # Calculate true percentages
    for t in parsed_tasks:
        t['percent'] = (t['time'] / total_time) * 100 if total_time > 0 else 0
        
    return parsed_tasks, total_time

def generate_pie_chart(tasks, total_time, output_file):
    if not tasks:
        print("No data found to plot")
        return

    # Configuration
    THRESHOLD_PERCENT = 2.0  # Hide label on chart if < 2%
    GROUP_OTHERS_THRESHOLD = 1.0 # Group into "Others" slice if < 1%
    
    # --- 1. Prepare Chart Data (Grouped) ---
    plot_labels = []
    plot_sizes = []
    
    # We need a color map that is consistent.
    # We will assign colors to the *Raw Tasks* first so they stick.
    cmap = plt.get_cmap("tab20c")
    # Generate enough colors for all tasks (cycling if needed, though tab20c is large)
    raw_colors = [cmap(i % 20) for i in range(len(tasks))]
    
    # Map task name to color
    task_color_map = {t['name']: raw_colors[i] for i, t in enumerate(tasks)}
    
    main_tasks_for_plot = []
    plot_colors = []
    
    others_time = 0
    others_count = 0
    
    for t in tasks:
        if t['percent'] >= GROUP_OTHERS_THRESHOLD:
            main_tasks_for_plot.append(t)
            plot_labels.append(t['name'] if t['percent'] >= THRESHOLD_PERCENT else "")
            plot_sizes.append(t['time'])
            plot_colors.append(task_color_map[t['name']])
        else:
            others_time += t['time']
            others_count += 1
            
    # Add Others slice if exists
    if others_time > 0:
        plot_labels.append(f"Others ({others_count})")
        plot_sizes.append(others_time)
        plot_colors.append('#D3D3D3') # Grey for Others

    # --- 2. Create Chart ---
    fig, ax = plt.subplots(figsize=(14, 8))
    
    explode = [0.05] + [0] * (len(plot_sizes) - 1)
    
    wedges, texts, autotexts = ax.pie(plot_sizes, explode=explode, labels=plot_labels, autopct='%1.1f%%',
                                      shadow=True, startangle=140, colors=plot_colors,
                                      pctdistance=0.85, labeldistance=1.05)
    
    # Clean up auto-percentages for small slices
    for i, a in enumerate(autotexts):
        # Calculate percent for this slice relative to total
        pct = (plot_sizes[i] / total_time) * 100
        if pct < THRESHOLD_PERCENT:
            a.set_text("")
        else:
            a.set_fontsize(9)
            a.set_fontweight('bold')
            a.set_color('white')

    # Draw center circle for Donut style
    centre_circle = plt.Circle((0,0), 0.70, fc='white')
    fig.gca().add_artist(centre_circle)
    
    # --- 3. Create Detailed Legend (Table-like, UNGROUPED) ---
    # User requested top 12+, we show top 16 to be safe and cover the "12 tasks"
    legend_handles = []
    display_limit = 16 
    
    for i, t in enumerate(tasks[:display_limit]):
        # Use the specific color assigned to this task
        color = task_color_map[t['name']]
        
        # Format: "Name : 16.5% (12345 ticks)"
        label_str = f"{t['name']:<15} : {t['percent']:>5.1f}% ({t['time']:,})"
        legend_handles.append(mpatches.Patch(color=color, label=label_str))

    ax.legend(handles=legend_handles, 
              title="Task Details (Top 16)",
              loc="center left",
              bbox_to_anchor=(1, 0, 0.5, 1),
              fontsize=9,
              title_fontsize=11)
    
    ax.set_title("System Task CPU Usage Analysis", fontsize=16, fontweight='bold')
    plt.tight_layout()
    
    # Save
    plt.savefig(output_file, dpi=150)
    print(f"Chart saved to {output_file}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python visualize_runtime_stats.py <log_file> <output_image>")
        sys.exit(1)
        
    input_log = sys.argv[1]
    output_img = sys.argv[2]
    
    tasks, total_time = parse_runtime_stats(input_log)
    generate_pie_chart(tasks, total_time, output_img)
