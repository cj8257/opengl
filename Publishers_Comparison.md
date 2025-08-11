# 数据发送器对比说明

本项目现在包含多个不同用途的数据发送器，每个都有特定的应用场景。

## 📊 发送器总览

### 1. **sender.cpp** (原始C++发送器)
```bash
./sender <数据文件夹路径> [是否循环:true|false]
```

**数据类型**: 多通道采样数据 (类似声学阵列数据)
- **格式**: 二进制数据 (4096字节/包)
- **内容**: 128通道 × 8采样点 × float32
- **模式**: ZeroMQ PUSH 
- **用途**: 发送预录制的二进制采样数据

### 2. **sender_data_publisher.py** (模拟sender.cpp)
```bash
python3 sender_data_publisher.py --mode normal
```

**数据类型**: 模拟多通道采样数据
- **格式**: 二进制数据 (4096字节/包)
- **内容**: 128通道 × 8采样点 × float32 (模拟生成)
- **模式**: ZeroMQ PUSH
- **用途**: 生成与sender.cpp格式兼容的模拟采样数据

### 3. **simple_sensor_publisher.py** (图表显示专用)
```bash
python3 simple_sensor_publisher.py --mode normal
```

**数据类型**: 传感器数据 (温度/湿度)
- **格式**: JSON字符串
- **内容**: 温度和湿度数值
- **模式**: ZeroMQ PUSH
- **用途**: **为SensorMonitorApp图表提供数据**

### 4. **sensor_data_publisher.py** (原始版本)
```bash
python3 sensor_data_publisher.py
```

**数据类型**: 传感器数据
- **格式**: JSON字符串  
- **内容**: 温度和湿度数值
- **模式**: ZeroMQ PUB (旧版本)
- **用途**: 兼容性保留

### 5. **advanced_sensor_publisher.py** (高级版本)
```bash
python3 advanced_sensor_publisher.py --mode normal
```

**数据类型**: 传感器数据
- **格式**: JSON字符串
- **内容**: 温度和湿度数值  
- **模式**: ZeroMQ PUB (旧版本)
- **用途**: 高级功能和配置选项

## 🎯 **推荐使用方案**

### 为图表显示发送数据
**使用**: `simple_sensor_publisher.py`
```bash
# 启动接收端
./SensorMonitor

# 启动数据发送 (另一个终端)
python3 simple_sensor_publisher.py --mode normal --interval 0.1
```

### 测试多通道数据处理
**使用**: `sender_data_publisher.py`
```bash
python3 sender_data_publisher.py --mode sine --interval 0.00355
```

### 发送真实二进制数据
**使用**: `sender.cpp`
```bash
./sender ./data_folder true
```

## 📋 **数据格式对比**

| 发送器 | 数据格式 | 大小 | 内容 | ZeroMQ模式 |
|--------|----------|------|------|------------|
| sender.cpp | 二进制 | 4096字节 | 128通道采样数据 | PUSH |
| sender_data_publisher.py | 二进制 | 4096字节 | 模拟128通道数据 | PUSH |
| simple_sensor_publisher.py | JSON | ~200字节 | 温度+湿度 | PUSH |
| sensor_data_publisher.py | JSON | ~200字节 | 温度+湿度 | PUB |
| advanced_sensor_publisher.py | JSON | ~250字节 | 温度+湿度+元数据 | PUB |

## 🔄 **ZeroMQ模式说明**

### PUSH-PULL模式 (推荐)
- **发送端**: PUSH socket
- **接收端**: PULL socket  
- **特点**: 负载均衡、可靠传输
- **适用**: sender.cpp, sender_data_publisher.py, simple_sensor_publisher.py

### PUB-SUB模式 (兼容性)
- **发送端**: PUB socket
- **接收端**: SUB socket
- **特点**: 发布-订阅、可能丢包
- **适用**: sensor_data_publisher.py, advanced_sensor_publisher.py

## 📈 **sender.cpp 具体数据格式**

### 数据结构
```
每个数据包 = 4096字节
├── Channel 0: 32字节 (8个float32采样点)
├── Channel 1: 32字节 (8个float32采样点)  
├── ...
└── Channel 127: 32字节 (8个float32采样点)
```

### 内存布局
```c++
struct DataPacket {
    float samples[128][8];  // 128通道 × 8采样点
};
// 总大小: 128 × 8 × 4 = 4096字节
```

### 发送参数
- **采样率**: 22500 Hz
- **每包采样点**: 8个
- **发送频率**: 2812.5包/秒
- **发送间隔**: ~355微秒

## 💡 **使用建议**

1. **图表显示**: 使用`simple_sensor_publisher.py`
2. **多通道测试**: 使用`sender_data_publisher.py`  
3. **真实数据**: 使用`sender.cpp`
4. **开发调试**: 任选JSON格式发送器

所有发送器都兼容更新后的SensorMonitorApp接收系统！