
"""
gen_chart_flash.py

Generates a Nested Pie / Sunburst Chart for Flash Memory Analysis.
- Inner Ring: Physical Partition Capability (The "Rooms")
- Outer Ring: Actual Usage vs Free Space (The "Furniture")

CONFIGURATION:
- FONT_SIZE_INNER: Font size for partition names (Inner ring).
- FONT_SIZE_OUTER: Font size for usage details (Outer ring).
- COLORS: Dictionary defining base colors for different partition types.

USAGE:
python scripts/gen_chart_flash.py --partitions partitions/v2/16m.csv --app_size <bytes> --asset_size <bytes> --output docs/images/resource_report
"""

import matplotlib.pyplot as plt
import argparse
import os
import sys
import numpy as np
from chart_utils import parse_partition_table

# ==========================================
# 1. VISUAL CONFIGURATION (User Modifiable)
# ==========================================

# Use ggplot style for a clean look
plt.style.use('ggplot')

# Font Settings
FONT_SIZE_TITLE = 16
FONT_SIZE_INNER = 10
FONT_SIZE_OUTER = 9
FONT_COLOR_INNER = 'black' # Color of text inside the inner ring (Partitions)

# [Q2 Configuration] Outer Ring Label Position
# 1.05 = Outside the ring
# 0.85 = Inside the ring (Centered in the band)
LABEL_DISTANCE_OUTER = 0.85 

# [Q3 Configuration] Legend Font Settings
LEGEND_FONT_SIZE = 13
LEGEND_TITLE = "Partition Map Details"

# Color Palette (Hex Codes or Matplotlib Names)
# We map partition keywords to base colors.
# Format: 'keyword': 'color_name'
COLOR_MAP_BASE = {
    'app_active': 'tab:blue',      # Active App - Blue
    'app_backup': 'tab:purple',    # Backup App - Purple (Distinct from Active)
    'assets':     'tab:orange',    # Assets - Orange
    'nvs':        'tab:green',     # NVS/Data - Green
    'free':       'silver',        # Unpartitioned - Grey
    'default':    'tab:olive'      # Others - Olive
}

# Opacity/Shading for Outer Ring
# Usage slices will use the SAME hue as inner ring but different brightness
ALPHA_USED = 0.8  # slightly transparent
ALPHA_FREE = 0.3  # very transparent (ghostly)

# ==========================================
# 2. GENERATION LOGIC
# ==========================================

def get_color(name):
    """Helper to pick color based on name."""
    name_lower = name.lower()
    if 'active' in name_lower or 'ota_0' in name_lower:
        return COLOR_MAP_BASE['app_active']
    if 'backup' in name_lower or 'ota_1' in name_lower:
        return COLOR_MAP_BASE['app_backup']
    if 'asset' in name_lower:
        return COLOR_MAP_BASE['assets']
    if 'nvs' in name_lower:
        return COLOR_MAP_BASE['nvs']
    if 'unallocated' in name_lower:
        return COLOR_MAP_BASE['free']
    return COLOR_MAP_BASE['default']

def generate_flash_chart(partitions, total_defined_size, app_used, asset_used, output_dir):
    if not partitions:
        print("Error: No partitions found.")
        return

    # Constants
    FLASH_TOTAL_MB = 16
    FLASH_TOTAL_BYTES = FLASH_TOTAL_MB * 1024 * 1024
    
    # Calculate free space (Unpartitioned)
    unpartitioned_space = FLASH_TOTAL_BYTES - total_defined_size
    if unpartitioned_space < 0: unpartitioned_space = 0

    # Data Structures for Plot
    inner_labels = []
    inner_sizes = []
    inner_colors = []
    
    outer_labels = []
    outer_sizes = []
    outer_colors = []
    
    # --- Helper to populate rings ---
    def add_partition(name, size, used_bytes=0, is_usage_known=False):
        # 1. INNER RING (Partition)
        inner_labels.append(name)
        inner_sizes.append(size)
        base_color = get_color(name)
        inner_colors.append(base_color)
        
        # 2. OUTER RING (Usage)
        if is_usage_known:
            free_bytes = size - used_bytes
            if free_bytes < 0: free_bytes = 0
            
            # Sub-slice 1: Used
            if used_bytes > 0:
                percent = (used_bytes / size) * 100
                outer_labels.append(f"Used\n{percent:.0f}%")
                outer_sizes.append(used_bytes)
                # Outer ring 'used' matches inner color but maybe slightly darker or same
                outer_colors.append(base_color) 
            
            # Sub-slice 2: Free
            if free_bytes > 0:
                percent = (free_bytes / size) * 100
                outer_labels.append(f"Free\n{percent:.0f}%")
                outer_sizes.append(free_bytes)
                # Free space is 'ghostly' version of base color or just grey
                outer_colors.append('#ecf0f1') # Light Grey for empty space
        else:
            # Usage unknown -> Treat as full block
            outer_labels.append("")
            outer_sizes.append(size)
            # Make it slightly transparent to show it's 'unknown/reserved'
            outer_colors.append(base_color) 

    # --- Process Partitions ---
    for p in partitions:
        p_name = p['name']
        p_size = p['size']
        
        # Rename logic for readability
        display_name = p_name
        used = 0
        known = False
        
        if p_name == 'ota_0': 
            display_name = 'App (Active)'
            used = app_used
            known = True
        elif p_name == 'ota_1':
            display_name = 'App (Backup)'
            used = app_used # Assume backup is roughly same size
            known = True
        elif p_name == 'assets':
            display_name = 'Assets'
            used = asset_used
            known = True
        else:
            # Treat nvs, phy, etc. as fully reserved
             used = p_size
             known = False 
             
        add_partition(display_name, p_size, used, known)

    # Add Unallocated Space
    if unpartitioned_space > 0:
        add_partition("Unallocated", unpartitioned_space, 0, False)

    # --- Plotting ---
    fig, ax = plt.subplots(figsize=(14, 10)) # Wider to accommodate legend
    ax.axis('equal')
    
    # Outer Ring (Usage)
    # Hide labels for tiny slices in outer ring too
    outer_labels_clean = []
    total_outer = sum(outer_sizes)
    for l, s in zip(outer_labels, outer_sizes):
        if s / total_outer < 0.02: # Hide if < 2%
            outer_labels_clean.append("")
        else:
            outer_labels_clean.append(l)

    # [Q2 Modification] Use LABEL_DISTANCE_OUTER (Set to 0.85 for inside)
    wedges_out, texts_out = ax.pie(outer_sizes, radius=1.0, labels=outer_labels_clean, 
                                   colors=outer_colors, labeldistance=LABEL_DISTANCE_OUTER, 
                                   startangle=90,
                                   textprops=dict(fontsize=FONT_SIZE_OUTER, color='black'))
    
    # Inner Ring (Partitions)
    # 1. Filter Labels for small partitions to prevent overlap
    inner_labels_display = []
    total_inner = sum(inner_sizes)
    for l, s in zip(inner_labels, inner_sizes):
        percent = s / total_inner
        if percent < 0.05: # Hide label if < 5%
            inner_labels_display.append("") 
        else:
            inner_labels_display.append(l)

    # 2. Change text color to black (or user configured)
    wedges_in, texts_in = ax.pie(inner_sizes, radius=0.7, labels=inner_labels_display, 
                                 colors=inner_colors, labeldistance=0.6,
                                 startangle=90,
                                 wedgeprops=dict(width=0.3, edgecolor='w'),
                                 textprops=dict(fontsize=FONT_SIZE_INNER, fontweight='bold', color=FONT_COLOR_INNER))
                                 
    # Style Adjustments
    plt.setp(texts_in, rotation_mode="anchor", ha="center", va="center")
    
    # Center White Circle (Donut Hole)
    centre_circle = plt.Circle((0,0), 0.4, fc='white')
    fig.gca().add_artist(centre_circle)
    
    # [Q3 Modification] Rich Legend with Size and %
    # Create custom handles for the legend from inner ring data
    import matplotlib.patches as mpatches
    legend_handles = []
    for name, color, size in zip(inner_labels, inner_colors, inner_sizes):
        # Calculate nice size string
        mb_size = size / (1024*1024)
        if mb_size >= 1.0:
            size_str = f"{mb_size:.2f}MB"
        else:
            size_str = f"{size/1024:.0f}KB"
            
        # Calculate percentage of total flash
        percent_str = f"{(size / FLASH_TOTAL_BYTES) * 100:.1f}%"
        
        # Format: "Name - Size (Percent)"
        label_text = f"{name} - {size_str} ({percent_str})"
        legend_handles.append(mpatches.Patch(color=color, label=label_text))
        
    ax.legend(handles=legend_handles, title=LEGEND_TITLE, 
              loc="center left", bbox_to_anchor=(1, 0, 0.5, 1),
              fontsize=LEGEND_FONT_SIZE, title_fontsize=LEGEND_FONT_SIZE+2)

    # Title
    ax.set_title(f'Flash: Partition Plan vs Usage ({FLASH_TOTAL_MB}MB)', fontsize=FONT_SIZE_TITLE, fontweight='bold')
    
    # Save
    out_path = os.path.join(output_dir, 'flash_sunburst.png')
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"Generated Flash Chart: {out_path}")

# ==========================================
# 3. MAIN ENTRY POINT
# ==========================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate Flash Sunburst Chart')
    parser.add_argument('--partitions', type=str, required=True, help='Path to partitions.csv')
    parser.add_argument('--app_size', type=int, default=0, help='Size of App binary in bytes')
    parser.add_argument('--asset_size', type=int, default=0, help='Size of Asset binary in bytes')
    parser.add_argument('--output', type=str, default='.', help='Output directory')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.output):
        os.makedirs(args.output)
        
    parts, total_size = parse_partition_table(args.partitions)
    generate_flash_chart(parts, total_size, args.app_size, args.asset_size, args.output)
