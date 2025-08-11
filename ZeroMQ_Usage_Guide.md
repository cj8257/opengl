# ZeroMQ 传感器数据通信系统使用指南

## 系统架构

本系统基于现有的 `sender.cpp` 和 `receiver.cpp` 的设计模式，使用 ZeroMQ PUSH-PULL 模式进行数据传输：

```
[Python Sender] --PUSH--> [C++ SensorMonitorApp] --PULL--> [图表显示]
     (端口5555)                 (接收端)
```

## 关键变更

### 1. C++ 接收端 (main.cpp)
- **模式变更**: 从 SUB 模式改为 PULL 模式
- **角色变更**: 从客户端连接改为服务器绑定
- **数据处理**: 支持 JSON 格式的传感器数据
- **日志输出**: 添加接收状态日志

### 2. Python 发送端 (sender_data_publisher.py)
- **模式采用**: 使用 PUSH 模式连接到接收端
- **数据格式**: 发送 JSON 格式的传感器数据
- **信号处理**: 支持优雅关闭
- **多种模式**: 支持5种数据生成模式

## 使用方法

### 步骤1: 启动接收端
```bash
cd /path/to/build
./SensorMonitor
```
程序将显示：
```
ZeroMQ receiver started, listening on port 5555
```

### 步骤2: 启动发送端
```bash
# 基本用法
python3 sender_data_publisher.py

# 高级用法
python3 sender_data_publisher.py --mode noisy --interval 0.05
```

## 数据格式

### JSON 数据包结构
```json
{
    "timestamp": "2024-01-01T12:00:00.123456",
    "temperature": 22.5,
    "humidity": 65.2,
    "sensor_id": "PUSH_SENSOR_001",
    "mode": "normal",
    "location": "ZeroMQ_Lab",
    "packet_id": 123
}
```

### 数据生成模式
1. **normal**: 正常模式，平滑变化
2. **noisy**: 噪声模式，随机波动
3. **extreme**: 极端模式，偶发异常值
4. **step**: 阶跃模式，突变数据
5. **sine**: 正弦波模式，规律变化

## 命令行选项

### sender_data_publisher.py 选项
```bash
python3 sender_data_publisher.py [options]

--address ADDRESS    ZeroMQ目标地址 (默认: tcp://127.0.0.1:5555)
--mode MODE         数据生成模式 (默认: normal)
--interval SECONDS  发送间隔秒数 (默认: 0.1)
```

### 使用示例
```bash
# 连接到不同地址
python3 sender_data_publisher.py --address tcp://192.168.1.100:5555

# 使用极端数据模式
python3 sender_data_publisher.py --mode extreme

# 高频发送 (50ms间隔)
python3 sender_data_publisher.py --interval 0.05

# 组合使用
python3 sender_data_publisher.py --mode step --interval 0.2
```

## 系统特性

### C++ 接收端特性
- **高性能**: 基于 ZeroMQ PULL 模式的高效接收
- **非阻塞**: 不影响 GUI 响应性的数据接收
- **错误处理**: JSON 解析错误捕获和日志输出
- **状态显示**: 实时连接状态和数据接收状态

### Python 发送端特性
- **可靠传输**: 基于 PUSH 模式的可靠数据推送
- **信号处理**: 支持 Ctrl+C 优雅关闭
- **灵活配置**: 多种命令行参数配置
- **数据模拟**: 5种不同的传感器数据模拟模式

## 故障排除

### 常见问题

1. **连接失败**
   - 确保接收端先启动
   - 检查防火墙设置
   - 验证端口5555可用

2. **数据不显示**
   - 检查JSON格式是否正确
   - 查看控制台错误信息
   - 确认数据包大小合理

3. **性能问题**
   - 调整发送间隔 `--interval`
   - 检查系统资源使用
   - 考虑降低数据发送频率

### 调试技巧

1. **查看接收日志**:
   ```bash
   ./SensorMonitor 2>&1 | grep "RECV"
   ```

2. **查看发送日志**:
   ```bash
   python3 sender_data_publisher.py --mode normal 2>&1 | grep "SEND"
   ```

3. **网络测试**:
   ```bash
   # 测试端口连通性
   telnet 127.0.0.1 5555
   ```

## 与原有系统的兼容性

- **保持兼容**: 旧的 `sensor_data_publisher.py` 仍然可用
- **新增功能**: `sender_data_publisher.py` 提供更多功能
- **架构升级**: 从 PUB-SUB 升级为 PUSH-PULL 模式
- **性能提升**: 更高效的数据传输和处理

## 开发扩展

### 添加新的传感器类型
在 JSON 数据包中添加新字段：
```python
sensor_data["pressure"] = generate_pressure()
sensor_data["light"] = generate_light_level()
```

### 自定义数据模式
继承并扩展 `DataMode` 枚举：
```python
class ExtendedDataMode(DataMode):
    CUSTOM = "custom"
```

### 集成真实硬件
替换数据生成函数：
```python
def read_real_sensor():
    return read_temperature_from_hardware()
```