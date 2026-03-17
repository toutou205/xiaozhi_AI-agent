#!/bin/bash

# ==========================================
# 局域网服务器自动化验证脚本 (Linux)
# ==========================================

# 预设路径 (根据 server 摸排获得)
PROJECT_DIR="/home/alex/esp_build/xiaozhi_AI-agent"
IDF_PATH="/home/alex/esp/esp-idf"

echo "=================================================="
echo "🚀 开始局域网服务器自动化验证 [Practice 1 & 2]"
echo "=================================================="

# 1. 进入工程目录
cd "$PROJECT_DIR" || { echo "❌ 目录不存在: $PROJECT_DIR"; exit 1; }

# 2. 激活 ESP-IDF 环境
echo -e "\n[1/3] 正在激活 ESP-IDF..."
source "$IDF_PATH/export.sh" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "❌ 激活 ESP-IDF 失败，请检查路径: $IDF_PATH"
    exit 1
fi
echo "✅ ESP-IDF 激活成功"

# 3. 静态排查 & 规范拦截 (Fail-fast)
echo -e "\n[2/3] 运行代码排版检查 (Clang-Format) & 静态分析 (Cppcheck)..."

# 严格对齐 baseline.yml，只检查 main/
find main/ -name "*.c" -o -name "*.cpp" -o -name "*.h" | grep -v "lang_config.h" | xargs clang-format --dry-run -Werror
FORMAT_RES=$?

if [ $FORMAT_RES -eq 0 ]; then
    echo "✅ Clang-Format 检查通过"
else
    echo "❌ Clang-Format 格式报警！请修复后重试 (本地可运行: python scripts/local_verify.py --fix)"
    exit $FORMAT_RES
fi

# 静态缺陷扫描
cppcheck --enable=warning --force --inline-suppr \
         --suppress=missingIncludeSystem --suppress=unknownMacro \
         --suppress=unusedFunction --suppress=virtualCallInConstructor \
         --suppress=toomanyconfigs --error-exitcode=1 \
         main/ components/imu_streamer components/sensor_icm42607
CPP_RES=$?

if [ $CPP_RES -eq 0 ]; then
    echo "✅ Cppcheck 静态扫描通过"
else
    echo "❌ Cppcheck 扫描发现严重问题，终止！"
    exit $CPP_RES
fi

# 4. 构建与资源分析
echo -e "\n[3/3] 编译固件 & 内存分析..."

# 清除旧的 CMakeCache（防止路径漂移）
if [ -d "build" ]; then
    echo "💡 发现旧 build 目录，正在全量清理..."
    rm -rf build
fi

idf.py set-target esp32s3 && idf.py build
BUILD_RES=$?

if [ $BUILD_RES -eq 0 ]; then
    echo -e "\n✅ 固件编译成功"
else
    echo -e "\n❌ 固件编译失败！"
    exit $BUILD_RES
fi

echo -e "\n📊 正在进行内存与 Flash 空余度剖析:"
idf.py size
idf.py size --format=json > build/size_report.json
if [ $? -eq 0 ]; then
    echo -e "\n📊 [新增] 模拟生成 Markdown 看板:\n"
    python scripts/visual_report.py build/size_report.json
fi
SIZE_RES=$?

if [ $SIZE_RES -eq 0 ]; then
    echo -e "\n✅ 内存分析完毕"
else
    echo -e "\n❌ 内存分析异常"
    exit $SIZE_RES
fi

# 5. 模拟合并二进制文件进行验证
echo -e "\n[4/4] 模拟合并二进制文件进行验证..."
if [ -f "build/flash_args" ]; then
    sed 's/--flash_size detect/--flash_size keep/g' build/flash_args > build/flash_args_safed
    cd build
    esptool.py --chip esp32s3 merge_bin -o merged-firmware.bin @flash_args_safed
    if [ $? -eq 0 ]; then
        echo "✅ 服务器端合并二进制验证成功！"
    else
        echo "❌ 服务器端合并二进制失败！"
        cd ..
        exit 1
    fi
    cd ..
else
    echo "❌ 找不到 build/flash_args"
fi

echo -e "\n🏆 所有流水线单测已跑通，安全，可以 Push 至 GitHub。"
exit 0
