import sys
import json

def main():
    if len(sys.argv) < 2:
        print("Usage: python scripts/visual_report.py size_report.json")
        sys.exit(1)
    
    try:
        with open(sys.argv[1], 'r') as f:
            content = f.read()
            # Find first '{' to skip headers
            start_idx = content.find('{')
            if start_idx != -1:
                content = content[start_idx:]
            data = json.loads(content)
    except Exception as e:
        print(f"Error reading JSON: {e}")
        sys.exit(1)
        
    print("## 📊 内存与 Flash 水位看板")
    print("| 内存类型 | 已用 (Bytes) | 总量 (Bytes) | 占用百分比 |")
    print("| :--- | :--- | :--- | :--- |")
    
    # S3 target usually uses diram (DRAM + IRAM shared)
    contains_diram = 'used_diram' in data and 'diram_total' in data
    contains_dram = 'used_dram' in data and 'dram_total' in data
    
    if contains_diram:
        ratio = data.get('used_diram_ratio', 0) * 100
        print(f"| **DRAM (Static RAM)** | `{data['used_diram']}` | `{data['diram_total']}` | `{ratio:.2f}%` |")
    elif contains_dram:
        ratio = data.get('used_dram_ratio', 0) * 100
        print(f"| **DRAM (Static RAM)** | `{data['used_dram']}` | `{data['dram_total']}` | `{ratio:.2f}%` |")
    else:
        print("| **DRAM (Static RAM)** | `N/A` | `N/A` | `N/A` |")
        
    # Flash
    if 'used_flash_non_ram' in data:
         print(f"| **Flash (Code + RoData)** | `{data['used_flash_non_ram']}` | - | - |")
    elif 'total_size' in data:
         # Fallback or general size
         print(f"| **Flash (Total Binary)** | `{data['total_size']}` | - | - |")
         
    print(f"\n**总固件大小**: `{data.get('total_size', 0)}` Bytes")

if __name__ == "__main__":
    main()
