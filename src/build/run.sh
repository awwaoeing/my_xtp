#!/bin/zsh
set -e  # 发生错误即停止脚本

echo "执行 cmake .."
cmake ..

echo "执行 make"
make

echo "运行 ./demo"
./demo

echo "运行 python latency_analyze.py"
python latency_analyze.py

