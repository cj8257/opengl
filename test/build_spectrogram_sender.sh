#!/bin/bash
###
 # @Author: chengjun 1@a.com
 # @Date: 2025-09-24 11:35:45
 # @LastEditors: chengjun 1@a.com
 # @LastEditTime: 2025-11-05 14:46:47
 # @FilePath: \SensorMonitorApp\build_spectrogram_sender.sh
 # @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
### 

echo "Building spectrogram sender..."

# 编译频谱数据发送器
g++ -std=c++17 -O2 -o spectrogram_sender spectrogram_sender.cpp -lpthread

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Usage: ./spectrogram_sender [host] [port]"
    echo "Default: ./spectrogram_sender 127.0.0.1 5556"
else
    echo "Build failed!"
    exit 1
fi