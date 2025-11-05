#!/bin/bash

echo "启动 Spectrogram Sender..."
./spectrogram_sender &
SPEC_PID=$!

echo "启动 Socket Sender..."
./sender_socket ./data true &
SOCKET_PID=$!

echo "两个发送器都已启动:"
echo "  Spectrogram Sender PID: $SPEC_PID"
echo "  Socket Sender PID: $SOCKET_PID"

echo "按 Ctrl+C 停止所有发送器"
trap "kill $SPEC_PID $SOCKET_PID; exit" INT

# 等待子进程
wait