# 快速测试指南

## 🚀 解决JSON解析错误

现在SensorMonitorApp支持智能数据格式检测，可以同时处理：
- **JSON传感器数据** (simple_sensor_publisher.py)
- **4096字节二进制数据** (sender_data_publisher.py)

## 🧪 测试步骤

### 测试1: JSON传感器数据 (推荐用于图表)

**终端1** - 启动接收端:
```bash
./SensorMonitor
```

**终端2** - 启动JSON数据发送:
```bash
python3 simple_sensor_publisher.py --mode normal --interval 0.1
```

**期望结果**:
- 控制台显示: `[RECV JSON] #10 | Size: ~200 bytes | Temp: 22.5°C | Humidity: 55.2%`
- 图表显示平滑的温度/湿度曲线

### 测试2: 二进制多通道数据

**终端1** - 启动接收端:
```bash
./SensorMonitor
```

**终端2** - 启动二进制数据发送:
```bash
python3 sender_data_publisher.py --mode sine --interval 0.00355
```

**期望结果**:
- 控制台显示: `[RECV BINARY] #100 | Size: 4096 bytes | Temp: 25.3°C | Humidity: 62.1%`
- 图表显示从多通道信号转换的传感器数据

### 测试3: 混合数据测试

可以同时运行多个发送器，SensorMonitorApp会自动识别并处理不同格式的数据！

## 📊 数据格式识别

SensorMonitorApp现在能自动识别：

| 数据类型 | 识别标志 | 处理方式 | 控制台标记 |
|----------|----------|----------|------------|
| JSON数据 | 以 `{` 开始 | JSON解析 | `[RECV JSON]` |
| 二进制数据 | 正好4096字节 | 二进制解析 | `[RECV BINARY]` |
| 未知数据 | 其他大小 | 跳过处理 | `[RECV UNKNOWN]` |

## 🔧 故障排除

### 如果仍然看到JSON解析错误：
1. 确保使用正确的发送器
2. 检查发送器是否连接到正确地址
3. 重新构建应用: `make`

### 数据格式不匹配：
- **想要图表显示**: 使用 `simple_sensor_publisher.py`
- **想要多通道测试**: 使用 `sender_data_publisher.py`
- **真实二进制数据**: 使用 `./sender data_folder`

## 💡 推荐配置

### 最佳图表体验：
```bash
# 启动应用
./SensorMonitor

# 另一个终端
python3 simple_sensor_publisher.py --mode normal --interval 0.1
```

### 高频数据测试：
```bash
# 启动应用
./SensorMonitor

# 另一个终端 (高频发送)
python3 sender_data_publisher.py --mode sine --interval 0.00355
```

现在JSON解析错误已经解决！🎉