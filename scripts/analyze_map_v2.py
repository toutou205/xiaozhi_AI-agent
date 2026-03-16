
import re
import sys

def parse_map_file(map_file_path):
    symbol_sizes = []
    
    # Regex to capture standard GCC map lines:
    # .text.funcname  0x0000000042000020       0x54 main/libmain.a(app_main.c.obj)
    # OR just symbol lines
    # We look for lines starting with space, having an address, a size, and an object file
    # Example:  0x0000000042002f50       0x48 main/libmain.a(app_main.c.obj)
    
    # Strategy: Find lines with Address + Size + Object
    # Pattern: ^ \s+ (0x[0-9a-f]+) \s+ (0x[0-9a-f]+) \s+ (.+)$
    
    pattern = re.compile(r'^\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(.+)$')
    
    with open(map_file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
        
    print(f"Total lines in map file: {len(lines)}")
    
    # Locate "Linker script and memory map" section to start parsing
    start_parsing = False
    
    for line in lines:
        if "Linker script and memory map" in line:
            start_parsing = True
            continue
            
        if not start_parsing:
            continue
            
        # Stop if we hit the cross reference table
        if "Cross Reference Table" in line:
            break
            
        match = pattern.match(line)
        if match:
            addr = int(match.group(1), 16)
            size = int(match.group(2), 16)
            name_and_obj = match.group(3).strip()
            
            # Filter out zero-size symbols and non-function sections if possible
            if size > 0:
                 # Clean up name: 
                 # .text.app_main or .scan_text
                 symbol_sizes.append({
                     'name': name_and_obj,
                     'size': size
                 })
                 
    # Sort by size desc
    symbol_sizes.sort(key=lambda x: x['size'], reverse=True)
    
    print("\nTop 20 Largest Symbols:")
    print(f"{'Size (Bytes)':<12} | {'Percentage':<10} | {'Symbol Info'}")
    print("-" * 80)
    
    total_size = sum(x['size'] for x in symbol_sizes)
    
    for i in range(min(20, len(symbol_sizes))):
        item = symbol_sizes[i]
        percent = (item['size'] / total_size) * 100
        print(f"{item['size']:<12} | {percent:.2f}%     | {item['name']}")

if __name__ == "__main__":
    parse_map_file("d:/Projecet/xiaozhi-esp32-2.1.0/build/xiaozhi.map")
