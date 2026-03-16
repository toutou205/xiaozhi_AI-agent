
"""
gen_chart_ram.py

Generates a Stacked Bar "Layer Cake" Chart for RAM Usage.
- Compares Internal SRAM vs External PSRAM.
- Segments: Static (Reserved), Dynamic (Heap Used), Free.

CONFIGURATION:
- COLORS: Dictionary defining colors for different memory segments.
- SRAM_TOTAL_KB: Total Internal SRAM size (default 512KB for S3).
- PSRAM_TOTAL_MB: Total External PSRAM size (default 16MB for Box-3).

USAGE:
python scripts/gen_chart_ram.py --log current_log.txt --output docs/images/resource_report
"""

import matplotlib.pyplot as plt
import argparse
import os
import sys
from chart_utils import parse_log_data

# ==========================================
# 1. VISUAL CONFIGURATION
# ==========================================
plt.style.use('ggplot')

# Hardware Constants
SRAM_TOTAL_KB = 512
PSRAM_TOTAL_MB = 16

# Colors (Hex or names)
COLORS = {
    'static':  '#2c3e50',  # Dark Blue/Grey - Foundation
    'dynamic': '#e67e22',  # Orange - Active Usage
    'free':    '#27ae60',  # Green - Available
    'ps_used': '#c0392b',  # Red - PSRAM Used
    'ps_free': '#2ecc71'   # Light Green - PSRAM Free
}

# ==========================================
# 2. GENERATION LOGIC
# ==========================================

def generate_ram_chart(heap_data, output_dir):
    if not heap_data or not heap_data.get('heap'):
        print("Error: No heap data found in log.")
        return
    
    heap = heap_data['heap']
    
    # 1. Internal SRAM Calculation (KB)
    heap_int_total = heap.get('HEAP_INT_TOTAL', 0)
    heap_int_free = heap.get('HEAP_INT_FREE', 0)
    
    # Static RAM = Total Physical - Heap Managed
    static_ram_used = (SRAM_TOTAL_KB * 1024) - heap_int_total
    if static_ram_used < 0: static_ram_used = 0
    
    dynamic_used = heap_int_total - heap_int_free
    
    s_static = static_ram_used / 1024
    s_dynamic = dynamic_used / 1024
    s_free = heap_int_free / 1024
    
    # 2. External PSRAM Calculation (MB)
    heap_ext_free = heap.get('HEAP_EXT_FREE', 0)
    psram_free_mb = heap_ext_free / (1024*1024)
    psram_used_mb = PSRAM_TOTAL_MB - psram_free_mb
    
    # --- Plotting ---
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 7))
    
    # Subplot 1: Internal SRAM
    bar_width = 0.5
    p1 = ax1.bar('SRAM', s_static, bar_width, label='Static/Reserved', color=COLORS['static'])
    p2 = ax1.bar('SRAM', s_dynamic, bar_width, bottom=s_static, label='Dynamic Heap', color=COLORS['dynamic'])
    p3 = ax1.bar('SRAM', s_free, bar_width, bottom=s_static+s_dynamic, label='Free', color=COLORS['free'])
    
    # Labels
    ax1.bar_label(p1, label_type='center', fmt='%.0f KB', color='white', fontweight='bold')
    ax1.bar_label(p2, label_type='center', fmt='%.0f KB', color='white', fontweight='bold')
    ax1.bar_label(p3, label_type='center', fmt='%.0f KB', color='white', fontweight='bold')
    
    # [Warning Line]
    # Rationale: 
    # - WiFi/BT needs ~50KB contiguous burst
    # - SSL Handshake needs ~30KB
    # - Fragmentation buffer ~20KB
    # Recommendation: Keep > 80KB free if possible, absolute min > 50KB
    THRESHOLD_WARNING_KB = 80
    ax1.axhline(y=SRAM_TOTAL_KB - THRESHOLD_WARNING_KB, color='red', linestyle='--', linewidth=2, label=f'Warning Threshold (Free < {THRESHOLD_WARNING_KB}KB)')

    ax1.set_ylabel('Size (KB)')
    ax1.set_title(f'Internal SRAM ({SRAM_TOTAL_KB}KB)')
    ax1.legend(loc='lower left')
    
    # Subplot 2: External PSRAM
    p4 = ax2.bar('PSRAM', psram_used_mb, bar_width, label='Used', color=COLORS['ps_used'])
    p5 = ax2.bar('PSRAM', psram_free_mb, bar_width, bottom=psram_used_mb, label='Free', color=COLORS['ps_free'])
    
    ax2.bar_label(p4, label_type='center', fmt='%.1f MB', color='white', fontweight='bold')
    ax2.bar_label(p5, label_type='center', fmt='%.1f MB', color='white', fontweight='bold')
    
    ax2.set_ylabel('Size (MB)')
    ax2.set_title(f'External PSRAM ({PSRAM_TOTAL_MB}MB)')
    ax2.legend()
    
    out_path = os.path.join(output_dir, 'ram_layer_cake.png')
    plt.suptitle('Memory Usage Baseline', fontsize=16)
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"Generated RAM Chart: {out_path}")

# ==========================================
# 3. MAIN ENTRY POINT
# ==========================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate RAM Stacked Bar Chart')
    parser.add_argument('--log', type=str, required=True, help='Path to log file')
    parser.add_argument('--output', type=str, default='.', help='Output directory')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.output):
        os.makedirs(args.output)
        
    with open(args.log, 'r') as f:
        log_content = f.read()
    
    data = parse_log_data(log_content)
    generate_ram_chart(data, args.output)
