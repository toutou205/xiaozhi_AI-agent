
"""
chart_utils.py

Shared utility functions for parsing ESP32 partition tables and runtime resource logs.
Used by: gen_chart_flash.py, gen_chart_ram.py, gen_chart_task.py
"""

import re
import csv
import os

def parse_partition_table(csv_path):
    """
    Parses partition table CSV to get Flash layout.
    
    Args:
        csv_path (str): Path to the partitions.csv file.
        
    Returns:
        tuple: (partitions_list, total_defined_size_bytes)
    """
    partitions = []
    total_size = 0
    try:
        with open(csv_path, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = [p.strip() for p in line.split(',')]
                if len(parts) >= 5:
                    name = parts[0]
                    size_str = parts[4]
                    
                    if 'K' in size_str.upper():
                        size = int(float(size_str.upper().replace('K', '')) * 1024)
                    elif 'M' in size_str.upper():
                        # Critical Fix: M = 1024*1024
                        size = int(float(size_str.upper().replace('M', '')) * 1024 * 1024)
                    else:
                        try:
                            size = int(size_str, 0)
                        except ValueError:
                            continue # Skip bad lines
                    
                    partitions.append({'name': name, 'size': size})
                    total_size += size
    except Exception as e:
        print(f"Error parsing partition table: {e}")
    
    return partitions, total_size

def parse_log_data(log_content):
    """
    Parses the cleanup log output (StartResourceMonitor).
    
    Args:
        log_content (str): The raw text content of the log file.
        
    Returns:
        dict: Dictionary containing 'heap' (dict) and 'tasks' (list).
    """
    data = {}
    
    # Heap Parsing
    data['heap'] = {}
    patterns = {
        'HEAP_INT_TOTAL': r'HEAP_INT_TOTAL: (\d+)',
        'HEAP_INT_FREE': r'HEAP_INT_FREE: (\d+)',
        'HEAP_INT_MAX_BLOCK': r'HEAP_INT_MAX_BLOCK: (\d+)',
        'HEAP_EXT_FREE': r'HEAP_EXT_FREE: (\d+)',
    }
    
    for key, pattern in patterns.items():
        match = re.search(pattern, log_content)
        if match:
            data['heap'][key] = int(match.group(1))

    # Task List Parsing
    tasks = []
    task_section = False
    
    for line in log_content.splitlines():
        line = line.strip()
        if "TASK_LIST:" in line:
            task_section = True
            continue
        if "--- END RESOURCE LOG ---" in line:
            task_section = False
            break
            
        if task_section and line:
            parts = line.split()
            # Heuristic: Name State Prio Stack Num
            # If stack (parts[-2]) is digit, assume valid
            if len(parts) >= 5 and parts[-2].isdigit():
                 tasks.append({
                     'name': parts[0],
                     'state': parts[1],
                     'prio': parts[2],
                     'stack_free': int(parts[-2]),
                     'num': parts[-1]
                 })
    
    data['tasks'] = tasks
    return data
