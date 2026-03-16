import os
import subprocess

clf = r"C:\Users\Boss\AppData\Roaming\Python\Python314\Scripts\clang-format.exe"
search_dirs = ["main", "components/imu_streamer", "components/sensor_icm42607"]

count = 0
for d in search_dirs:
    if not os.path.exists(d): continue
    for root, dirs, files in os.walk(d):
        for f in files:
            if f.endswith(('.c', '.cpp', '.h')):
                path = os.path.join(root, f)
                subprocess.run([clf, "-i", path])
                count += 1

print(f"Successfully formatted {count} files.")
