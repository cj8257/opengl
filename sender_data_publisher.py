#!/usr/bin/env python3
"""
基于sender.cpp的多通道采样数据发送器
使用ZeroMQ PUSH模式向SensorMonitorApp发送二进制采样数据
模拟sender.cpp的数据格式：128通道 × 8采样点 × 4字节 = 4096字节/包
"""

import zmq
import struct
import time
import math
import random
import argparse
from datetime import datetime
from enum import Enum
import signal
import sys
import numpy as np

class DataMode(Enum):
    NORMAL = "normal"           # 正常模式：平滑变化
    NOISY = "noisy"            # 噪声模式：加入更多随机噪声
    EXTREME = "extreme"         # 极端模式：偶尔产生极端值
    STEP = "step"              # 阶跃模式：突然跳跃变化
    SINE_WAVE = "sine"         # 正弦波模式：规律性变化

# sender.cpp的数据格式常量
POINT_SIZE = 4                    # 每个采样点的字节数 (float32)
POINT_COUNT = 128                 # 阵元数量
PACKET_POINT_COUNT = 8            # 每个数据包的采样点数
PACKET_BYTE_SIZE = PACKET_POINT_COUNT * POINT_SIZE * POINT_COUNT  # 4096字节
FS = 22500                        # 采样率

class MultichannelDataSender:
    def __init__(self, address="tcp://127.0.0.1:5555", mode=DataMode.NORMAL, interval=None):
        self.address = address
        self.mode = mode
        self.time_counter = 0.0
        self.running = True
        self.send_count = 0
        
        # 模拟sender.cpp的发送周期（纳秒）
        send_period_ns = 1_000_000_000 // (FS // PACKET_POINT_COUNT)
        self.interval = interval if interval else send_period_ns / 1_000_000_000  # 转换为秒
        
        # 初始化 ZeroMQ (基于sender.cpp的PUSH模式)
        self.context = zmq.Context()
        self.sender = self.context.socket(zmq.PUSH)
        
        # 连接到接收端
        try:
            self.sender.connect(self.address)
            print(f"Connected to {self.address}")
        except Exception as e:
            print(f"Failed to connect to {self.address}: {e}")
            sys.exit(1)
        
        # 注册信号处理
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
        
    def signal_handler(self, signum, frame):
        """处理终止信号"""
        print(f"\nReceived signal {signum}. Shutting down gracefully...")
        self.running = False
        
    def generate_multichannel_data(self):
        """生成多通道采样数据 (模拟sender.cpp的数据格式)"""
        # 创建 128通道 × 8采样点 的数据矩阵
        data = np.zeros((POINT_COUNT, PACKET_POINT_COUNT), dtype=np.float32)
        
        for channel in range(POINT_COUNT):
            for sample in range(PACKET_POINT_COUNT):
                time_offset = self.time_counter + sample * (1.0 / FS)
                
                if self.mode == DataMode.NORMAL:
                    # 每个通道不同的频率分量
                    freq = 100 + channel * 0.5  # 基频从100Hz开始递增
                    amplitude = 1.0 + 0.01 * channel
                    signal = amplitude * math.sin(2 * math.pi * freq * time_offset)
                    noise = random.uniform(-0.1, 0.1)
                    data[channel, sample] = signal + noise
                    
                elif self.mode == DataMode.NOISY:
                    freq = 200 + channel * 1.0
                    amplitude = 0.8 + 0.02 * channel  
                    signal = amplitude * math.sin(2 * math.pi * freq * time_offset)
                    noise = random.uniform(-0.5, 0.5)
                    data[channel, sample] = signal + noise
                    
                elif self.mode == DataMode.EXTREME:
                    if random.random() < 0.01:  # 1%概率产生极端值
                        data[channel, sample] = random.choice([-10.0, 10.0])
                    else:
                        freq = 150 + channel * 0.8
                        amplitude = 1.2 + 0.015 * channel
                        signal = amplitude * math.sin(2 * math.pi * freq * time_offset)
                        data[channel, sample] = signal
                        
                elif self.mode == DataMode.STEP:
                    step_period = 100  # 每100个包改变一次
                    step_value = int(self.send_count / step_period) % 4
                    amplitudes = [0.5, 1.0, 1.5, 2.0]
                    amplitude = amplitudes[step_value]
                    freq = 120 + channel * 0.6
                    data[channel, sample] = amplitude * math.sin(2 * math.pi * freq * time_offset)
                    
                elif self.mode == DataMode.SINE_WAVE:
                    # 纯正弦波，每个通道不同相位
                    freq = 80
                    phase = channel * (2 * math.pi / POINT_COUNT)  # 均匀分布相位
                    amplitude = 1.0
                    data[channel, sample] = amplitude * math.sin(2 * math.pi * freq * time_offset + phase)
        
        return data
    
    def create_binary_data_packet(self):
        """创建二进制数据包 (模拟sender.cpp的格式)"""
        # 生成多通道数据
        multichannel_data = self.generate_multichannel_data()
        
        # 转换为二进制数据包 (与sender.cpp相同的内存布局)
        # 数据按 [Channel0_Sample0...Sample7][Channel1_Sample0...Sample7]... 排列
        binary_packet = bytearray()
        
        for channel in range(POINT_COUNT):
            for sample in range(PACKET_POINT_COUNT):
                # 将float32数值转换为4字节二进制
                float_bytes = struct.pack('f', multichannel_data[channel, sample])
                binary_packet.extend(float_bytes)
        
        # 验证数据包大小
        assert len(binary_packet) == PACKET_BYTE_SIZE, f"Invalid packet size: {len(binary_packet)}"
        
        return binary_packet
    
    def send_data(self):
        """发送数据包 (模拟sender.cpp的发送方式)"""
        try:
            # 创建二进制数据包
            binary_packet = self.create_binary_data_packet()
            
            # 发送二进制数据 (使用PUSH socket, 与sender.cpp相同)
            self.sender.send(binary_packet, zmq.NOBLOCK)
            
            self.send_count += 1
            
            # 模仿sender.cpp的日志格式
            print(f"[SEND] Packet #{self.send_count} | "
                  f"Size: {len(binary_packet)} bytes")
            
            return True
            
        except zmq.Again:
            print("Warning: Send would block, skipping packet")
            return False
        except Exception as e:
            print(f"Failed to send data: {e}")
            return False
    
    def run(self):
        """主运行循环 (模仿sender.cpp的发送逻辑)"""
        print("=" * 80)
        print("Multichannel Data Sender (based on sender.cpp)")
        print("=" * 80)
        print(f"Target address: {self.address}")
        print(f"Data mode: {self.mode.value}")
        print(f"Send interval: {self.interval:.6f} seconds")
        print(f"Packet size: {PACKET_BYTE_SIZE} bytes ({POINT_COUNT} channels × {PACKET_POINT_COUNT} samples)")
        print("Press Ctrl+C to stop")
        print("-" * 80)
        
        try:
            while self.running:
                # 发送数据
                if self.send_data():
                    self.time_counter += self.interval
                
                # 模仿sender.cpp的定时发送
                time.sleep(self.interval)
                
        except Exception as e:
            print(f"Error in main loop: {e}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        """清理资源 (模仿sender.cpp的清理逻辑)"""
        print(f"\nSender finished. Total packets sent: {self.send_count}")
        
        if hasattr(self, 'sender'):
            self.sender.close()
        if hasattr(self, 'context'):
            self.context.term()
        
        print("ZeroMQ resources cleaned up")

def main():
    parser = argparse.ArgumentParser(
        description="Multichannel Data Sender (based on sender.cpp binary format)"
    )
    parser.add_argument("--address", default="tcp://127.0.0.1:5555", 
                       help="ZeroMQ target address (default: tcp://127.0.0.1:5555)")
    parser.add_argument("--mode", choices=[mode.value for mode in DataMode], 
                       default=DataMode.NORMAL.value, help="Data generation mode")
    parser.add_argument("--interval", type=float, default=None, 
                       help="Send interval in seconds (default: auto from FS)")
    
    args = parser.parse_args()
    
    mode = DataMode(args.mode)
    sender = MultichannelDataSender(address=args.address, mode=mode, interval=args.interval)
    sender.run()

if __name__ == "__main__":
    main()