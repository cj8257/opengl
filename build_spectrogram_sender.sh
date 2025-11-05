#!/bin/bash

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