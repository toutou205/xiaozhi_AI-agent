#修改建议 1.饼状图图形背景颜色和数字相近，导致很难区分。 2. 由于所占百分比太小，'Bootloader', 'NVS/PartTable'现实的时候有重叠，需要将位置进行一个错位

import matplotlib.pyplot as plt
import numpy as np

# ==========================================
# 1. 数据准备 (Data Preparation)
# ==========================================

# ---------------- Flash Memory Data ----------------
FLASH_TOTAL_MB = 16
FLASH_TOTAL_BYTES = FLASH_TOTAL_MB * 1024 * 1024

# 从日志提取的真实数据
app_size = 2967511        # xiaozhi.bin
bootloader_size = 16464   # 0x4050
partition_table = 0x1000  # 假设标准大小 4KB
nvs_size = 0x4000         # 假设标准大小 16KB (常见配置)
filesystem_size = 4 * 1024 * 1024 # 假设分配了 4MB 给文件系统 (SPIFFS/LittleFS)

# 计算剩余空间
used_flash = app_size + bootloader_size + partition_table + nvs_size + filesystem_size
free_flash = FLASH_TOTAL_BYTES - used_flash

# ---------------- SRAM Data ----------------
SRAM_TOTAL_KB = 512
SRAM_TOTAL_BYTES = SRAM_TOTAL_KB * 1024

# 从日志提取的静态内存 (DIRAM Used: .text + .data + .bss)
# 注意: .text (IRAM) 实际上也占用内部 SRAM
static_ram_used = 169147 

# --- 随机生成动态数据 (Simulated Dynamic Data) ---
# 模拟系统运行时的堆内存占用 (WiFi buffer, Audio Ringbuffer, Task Stacks)
import random
dynamic_heap_used = random.randint(100 * 1024, 200 * 1024) # 随机 100KB - 200KB

free_sram = SRAM_TOTAL_BYTES - static_ram_used - dynamic_heap_used

# ==========================================
# 2. 绘图逻辑 (Plotting)
# ==========================================

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
plt.style.use('ggplot') # 使用更好看的样式

# --- Chart 1: Flash Usage Pie Chart ---
flash_labels = ['App Code (Xiaozhi)', 'Filesystem (Assets)', 'Bootloader', 'NVS/PartTable', 'Free Space']
flash_sizes = [app_size, filesystem_size, bootloader_size, partition_table + nvs_size, free_flash]
flash_colors = ['#3498db', '#9b59b6', '#95a5a6', '#f1c40f', '#ecf0f1']
explode = (0.1, 0.1, 0, 0, 0)  # 炸开 "App Code" 扇区

wedges, texts, autotexts = ax1.pie(flash_sizes, explode=explode, labels=flash_labels, autopct='%1.1f%%',
                                   shadow=True, startangle=140, colors=flash_colors, textprops={'fontsize': 10})

ax1.set_title(f'ESP32-S3 Flash Usage (Total {FLASH_TOTAL_MB}MB)', fontsize=14, fontweight='bold')
plt.setp(autotexts, size=9, weight="bold", color="white")

# --- Chart 2: SRAM Usage Stacked Bar Chart ---
# 转换单位为 KB
sram_static_kb = static_ram_used / 1024
sram_dynamic_kb = dynamic_heap_used / 1024
sram_free_kb = free_sram / 1024

bar_width = 0.5
p1 = ax2.bar('SRAM Usage', sram_static_kb, bar_width, label='Static (Global/BSS)', color='#2c3e50')
p2 = ax2.bar('SRAM Usage', sram_dynamic_kb, bar_width, bottom=sram_static_kb, label='Dynamic Heap (Runtime)', color='#e67e22')
p3 = ax2.bar('SRAM Usage', sram_free_kb, bar_width, bottom=sram_static_kb + sram_dynamic_kb, label='Free SRAM', color='#bdc3c7')

# 添加数值标注
ax2.bar_label(p1, label_type='center', fmt='%.0f KB', color='white', fontweight='bold')
ax2.bar_label(p2, label_type='center', fmt='%.0f KB', color='white', fontweight='bold')
ax2.bar_label(p3, label_type='center', fmt='%.0f KB', color='black', fontweight='bold')

# 设置警戒线 (80% Usage)
ax2.axhline(y=SRAM_TOTAL_KB * 0.8, color='red', linestyle='--', linewidth=1, label='80% Warning Line')

ax2.set_ylabel('Memory Size (KB)')
ax2.set_title(f'Internal SRAM Distribution (Total {SRAM_TOTAL_KB}KB)', fontsize=14, fontweight='bold')
ax2.set_ylim(0, SRAM_TOTAL_KB + 50)
ax2.legend(loc='upper right')

# ==========================================
# 3. 输出与保存
# ==========================================
plt.tight_layout()
print(f"[Simulated Data] Dynamic Heap set to: {sram_dynamic_kb:.2f} KB")
print(f"[Real Data] Static RAM used: {sram_static_kb:.2f} KB")
print(f"[Real Data] App Flash size: {app_size/1024/1024:.2f} MB")

# 如果你在 Jupyter Notebook 中，这一行会直接显示图片
plt.show()