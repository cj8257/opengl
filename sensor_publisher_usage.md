# 传感器数据发布器使用说明

本项目包含两个Python脚本用于模拟传感器数据并通过ZeroMQ发送给SensorMonitorApp。

## 文件说明

### 1. sensor_data_publisher.py
基础版本的传感器数据发布器，发送温度和湿度数据。

**使用方法：**
```bash
python3 sensor_data_publisher.py
```

**数据格式：**
```json
{
    "timestamp": "2024-01-01T12:00:00.123456",
    "temperature": 22.5,
    "humidity": 65.2,
    "sensor_id": "TEMP_HUMID_001",
    "location": "Room_A"
}
```

### 2. advanced_sensor_publisher.py
高级版本，支持多种数据生成模式和配置选项。

**基本使用：**
```bash
python3 advanced_sensor_publisher.py
```

**高级选项：**
```bash
# 指定端口
python3 advanced_sensor_publisher.py --port 5556

# 指定数据模式
python3 advanced_sensor_publisher.py --mode noisy

# 指定发送间隔
python3 advanced_sensor_publisher.py --interval 0.05

# 组合使用
python3 advanced_sensor_publisher.py --port 5555 --mode extreme --interval 0.1
```

## 数据模式说明

1. **normal** (默认): 正常模式，数据变化平滑
   - 温度: 15°C - 29°C 范围
   - 湿度: 38% - 72% 范围

2. **noisy**: 噪声模式，包含更多随机变化
   - 增加随机噪声幅度
   - 模拟真实环境的不稳定性

3. **extreme**: 极端模式，偶尔产生异常值
   - 5%概率产生极端温度值 (5°C 或 38°C)
   - 5%概率产生极端湿度值 (10% 或 95%)

4. **step**: 阶跃模式，数据突然跳跃变化
   - 温度在4个固定值间切换: 18°C, 22°C, 26°C, 30°C
   - 湿度在3个固定值间切换: 40%, 60%, 80%

5. **sine**: 正弦波模式，规律性变化
   - 纯数学函数生成的规律数据
   - 适合测试图表显示效果

## 依赖库安装

```bash
# 安装ZeroMQ Python绑定
pip3 install pyzmq

# 或使用conda
conda install pyzmq
```

## 运行流程

1. 首先启动SensorMonitorApp
```bash
cd /path/to/build
./SensorMonitor
```

2. 然后启动数据发布器
```bash
# 基础版本
python3 sensor_data_publisher.py

# 或高级版本
python3 advanced_sensor_publisher.py --mode normal
```

## 故障排除

### 连接问题
- 确保防火墙允许5555端口通信
- 检查ZeroMQ是否正确安装
- 确认SensorMonitorApp正在运行并监听正确端口

### 数据格式问题
- 发布器发送JSON格式数据
- SensorMonitorApp期望包含"temperature"和"humidity"字段
- 检查JSON格式是否正确

### 性能问题
- 默认发送间隔为100ms (0.1秒)
- 可通过--interval参数调整
- 过高的发送频率可能影响性能

## 自定义开发

可以基于这些脚本开发自己的数据源：

1. 修改数据生成函数
2. 添加新的传感器类型
3. 集成真实硬件传感器
4. 添加数据记录功能

示例：添加压力传感器
```python
sensor_data["pressure"] = generate_pressure()  # 添加到JSON数据中
```