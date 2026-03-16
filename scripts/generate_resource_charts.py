
"""
generate_resource_charts.py

MASTER SCRIPT / WRAPPER
This script orchestrates the generation of all system resource charts by calling the specialized sub-scripts:
1. scripts/gen_chart_flash.py
2. scripts/gen_chart_ram.py
3. scripts/gen_chart_task.py

USAGE:
python scripts/generate_resource_charts.py --partitions partitions/v2/16m.csv --log current_log.txt --app_size 2972135 --asset_size 1883624 --output docs/images/resource_report
"""

import argparse
import subprocess
import sys
import os

def main():
    parser = argparse.ArgumentParser(description='Generate All Resource Charts')
    parser.add_argument('--partitions', type=str, required=True)
    parser.add_argument('--log', type=str, required=True)
    parser.add_argument('--output', type=str, default='.')
    parser.add_argument('--app_size', type=str, default='0')
    parser.add_argument('--asset_size', type=str, default='0')
    # Backward compatibility
    parser.add_argument('--demo', action='store_true', help='Ignored in new version')

    args = parser.parse_args()
    
    python_exe = sys.executable
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    build_dir = os.path.join(project_root, 'build')

    # --- Auto-Detect Logic ---
    app_size_final = args.app_size
    asset_size_final = args.asset_size

    # If App Size not provided (or 0), try to find build/xiaozhi.bin
    if str(app_size_final) == '0':
        bin_path = os.path.join(build_dir, 'xiaozhi.bin')
        if os.path.exists(bin_path):
            size = os.path.getsize(bin_path)
            print(f"Auto-detected App Size from {bin_path}: {size} bytes")
            app_size_final = str(size)
        else:
            print(f"Warning: Could not find {bin_path}, using 0.")

    # If Asset Size not provided (or 0), try to find build/assets.bin
    if str(asset_size_final) == '0':
        bin_path = os.path.join(build_dir, 'assets.bin')
        if os.path.exists(bin_path):
            size = os.path.getsize(bin_path)
            print(f"Auto-detected Asset Size from {bin_path}: {size} bytes")
            asset_size_final = str(size)
        else:
            print(f"Warning: Could not find {bin_path}, using 0.")

    # 1. Flash Chart
    print("\n[1/3] Generating Flash Chart...")
    subprocess.run([
        python_exe, os.path.join(script_dir, 'gen_chart_flash.py'),
        '--partitions', args.partitions,
        '--app_size', str(app_size_final),
        '--asset_size', str(asset_size_final),
        '--output', args.output
    ], check=True)
    
    # 2. RAM Chart
    print("\n[2/3] Generating RAM Chart...")
    subprocess.run([
        python_exe, os.path.join(script_dir, 'gen_chart_ram.py'),
        '--log', args.log,
        '--output', args.output
    ], check=True)
    
    # 3. Task Chart
    print("\n[3/3] Generating Task Chart...")
    subprocess.run([
        python_exe, os.path.join(script_dir, 'gen_chart_task.py'),
        '--log', args.log,
        '--output', args.output
    ], check=True)
    
    print("\nAll charts generated successfully!")

if __name__ == "__main__":
    main()
