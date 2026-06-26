#!/bin/bash

# 遭遇任何错误时立即终止脚本
set -e

# 定义终端 ANSI 颜色
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # 无颜色 (Reset)

echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}      AlgoRepo CLI - Auto Build Script     ${NC}"
echo -e "${BLUE}===========================================${NC}"

# 检查 CMake 是否可用
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}[ERROR] CMake is not installed or not in PATH.${NC}"
    exit 1
fi

BUILD_DIR="build"
CONFIG="Release"

# 检查是否传入了 clean 参数
if [ "$1" = "clean" ]; then
    echo -e "${YELLOW}[INFO] Cleaning old build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# 配置 CMake 项目
echo -e "${BLUE}[INFO] Configuring project with CMake...${NC}"
cmake -B "$BUILD_DIR" -S . -DCMAKE_BUILD_TYPE=$CONFIG

# 执行编译
echo -e "${BLUE}[INFO] Building project ($CONFIG)...${NC}"
cmake --build "$BUILD_DIR" --config $CONFIG

# 大功告成
echo -e ""
echo -e "${GREEN}[SUCCESS] Build completed successfully.${NC}"
echo -e "${BLUE}[INFO] Target binaries are located in ./\$BUILD_DIR/${NC}"
echo -e ""
