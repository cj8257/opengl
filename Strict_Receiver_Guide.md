# 严格按照receiver.cpp的数据接收指南

## 🎯 **完全按照receiver.cpp重构**

现在main.cpp已经严格按照receiver.cpp的方式重写，完全模拟其数据处理逻辑：

### 📊 **数据处理特性**

1. **严格的数据包验证**
   - 只接受4096字节的数据包
   - 其他大小的数据包会报错并丢弃

2. **多通道采样数据缓存**
   - 128个通道，每个通道独立缓存
   - 每包8个采样点，最多缓存10000个采样点/通道

3. **receiver.cpp格式的日志输出**
   ```
   [RECV] #1 | Size: 4096 bytes
   Channel 0 (last 8 samples): 1.23 -0.45 2.67 -1.89 ...
   ```

4. **程序结束时保存缓存数据**
   - 保存到 `cached_samples.bin` 文件
   - 打印每个通道的采样数量统计

## 🚀 **使用方法**

### 步骤1: 启动接收端
```bash
./SensorMonitor
```
**期望输出**:
```
ZeroMQ receiver started, listening on port 5555
```

### 步骤2: 启动sender数据发送
```bash
# 使用二进制多通道数据发送器
python3 sender_data_publisher.py --mode sine --interval 0.00355
```

### 步骤3: 观察数据处理
**控制台输出** (严格按照receiver.cpp格式):
```
[RECV] #1 | Size: 4096 bytes
Channel 0 (last 8 samples): 1.0000 0.8090 0.3090 -0.3090 -0.8090 -1.0000 -0.8090 -0.3090 
[RECV] #2 | Size: 4096 bytes
Channel 0 (last 8 samples): 0.3090 0.8090 1.0000 0.8090 0.3090 -0.3090 -0.8090 -1.0000 
...
```

**图表显示**:
- 温度数据来源: 通道0的最新采样值
- 湿度数据来源: 通道1的最新采样值
- 采样值自动映射到传感器范围

## 📋 **数据处理流程**

### 1. 数据包接收 (4096字节)
```cpp
std::vector<uint8_t> buffer(PACKAGE_SIZE);  // 4096字节缓冲区
int recv_size = zmq_recv(receiver, buffer.data(), buffer.size(), ZMQ_DONTWAIT);
```

### 2. 数据包验证
```cpp
if (recv_size != static_cast<int>(PACKAGE_SIZE)) {
    std::cerr << "Received unexpected packet size: " << recv_size 
              << " (expected " << PACKAGE_SIZE << ")" << std::endl;
}
```

### 3. 多通道数据解码
```cpp
const float* samples = reinterpret_cast<const float*>(buffer.data());
size_t sample_count = PACKAGE_SIZE / sizeof(float); // 1024个float
```

### 4. 按通道缓存数据
```cpp
for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
    for (size_t sample = 0; sample < SAMPLES_PER_PACKET; ++sample) {
        size_t index = channel * SAMPLES_PER_PACKET + sample;
        channel_samples[channel].push_back(samples[index]);
    }
}
```

### 5. 转换为传感器数据
```cpp
float raw_temp = channel_samples[0].back();   // 通道0 → 温度
float raw_humid = channel_samples[1].back();  // 通道1 → 湿度
current_temperature = 22.0f + raw_temp * 8.0f;
current_humidity = 50.0f + raw_humid * 25.0f;
```

## 🔧 **与receiver.cpp的一致性**

| 特性 | receiver.cpp | main.cpp | 一致性 |
|------|-------------|----------|--------|
| 数据包大小检查 | ✅ | ✅ | 完全一致 |
| 多通道数据缓存 | ✅ | ✅ | 完全一致 |
| 日志输出格式 | ✅ | ✅ | 完全一致 |
| 通道数据打印 | ✅ | ✅ | 完全一致 |
| 缓存数据保存 | ✅ | ✅ | 完全一致 |
| ZeroMQ PULL模式 | ✅ | ✅ | 完全一致 |

## 📈 **程序结束时的输出**

当您关闭程序时，会看到与receiver.cpp完全相同的输出：

```
Saved cached samples to cached_samples.bin
Cache summary:
Channel 0: 2584 samples
Channel 1: 2584 samples
Channel 2: 2584 samples
...
Channel 127: 2584 samples
Receiver finished. Total packets received: 323
```

## 💾 **缓存文件格式**

保存的 `cached_samples.bin` 文件格式与receiver.cpp完全相同：
```
[Channel Index][Sample Count][Sample Data...]
```

## ⚠️ **注意事项**

1. **只接受4096字节数据包** - 其他大小会被拒绝
2. **使用sender_data_publisher.py** - 发送正确格式的二进制数据
3. **高频数据传输** - 采样率22500Hz，发送频率2812.5包/秒
4. **内存管理** - 每通道最多缓存10000个采样点

现在main.cpp完全按照receiver.cpp的标准工作！🎉