// 包含DataManager的头文件
#include "Core/DataManager.h"
// 包含算法库的头文件
#include <algorithm>
// 包含chrono库的头文件，用于时间相关的操作
#include <chrono>
// 包含输入输出流库的头文件
#include <iostream>

// DataManager类的构造函数
DataManager::DataManager() {
    // 调整原始通道数据容器的大小以容纳所有通道
    raw_channel_data.resize(CHANNEL_COUNT);
    // 调整通道显示数据容器的大小以容纳所有通道
    channel_display_data.resize(CHANNEL_COUNT);
    
    // 为每个原始数据通道预留内存空间
    for (auto& channel : raw_channel_data) {
        channel.reserve(10000);
    }
    
    // 为每个显示数据通道预留内存空间
    for (auto& channel : channel_display_data) {
        channel.reserve(MAX_DISPLAY_SAMPLES);
    }
    
    // 为时间值容器预留内存空间
    time_values.reserve(MAX_DISPLAY_SAMPLES);
    
    // 创建并启动一个新线程来处理数据
    processing_thread = std::thread(&DataManager::processData, this);
}

// DataManager类的析构函数
DataManager::~DataManager() {
    // 设置停止标志为true，以通知处理线程退出
    should_stop = true;
    // 如果处理线程是可加入的
    if (processing_thread.joinable()) {
        // 等待处理线程执行完毕
        processing_thread.join();
    }
}

// 添加单个数据点的方法（当前未使用）
void DataManager::addData(const DataPoint& point) {
    // 使用互斥锁保护数据访问
    std::lock_guard<std::mutex> lock(data_mutex);
    // 如果缓冲区已满，则移除最旧的数据
    if (buffer.size() >= maxSize)
        buffer.erase(buffer.begin());
    // 将新数据点添加到缓冲区
    buffer.push_back(point);
}

// 添加通道数据的方法
void DataManager::addChannelData(const std::vector<std::vector<float>>& channel_samples, double base_timestamp) {
    // 使用互斥锁保护数据访问
    std::lock_guard<std::mutex> lock(data_mutex);
    
    // 检查传入的通道数是否正确
    if (channel_samples.size() != CHANNEL_COUNT) return;
    
    // 遍历每个通道
    for (size_t ch = 0; ch < CHANNEL_COUNT; ++ch) {
        // 获取当前通道的样本数据
        const auto& samples = channel_samples[ch];
        // 遍历当前通道的所有样本
        for (const float sample : samples) {
            // 如果原始数据通道的样本数超过上限
            if (raw_channel_data[ch].size() >= 50000) {
                // 移除最旧的样本
                raw_channel_data[ch].erase(raw_channel_data[ch].begin());
            }
            // 将新样本添加到原始数据通道
            raw_channel_data[ch].push_back(sample);
        }
    }
    
    // 更新接收到的总样本数
    total_samples_received += (channel_samples.empty() ? 0 : channel_samples[0].size());
}

// 新增：处理二进制数据包的方法
void DataManager::addBinaryPacket(const std::vector<uint8_t>& packet_data) {
    // 检查数据包大小是否有效
    if (packet_data.size() != PACKAGE_SIZE) {
        // 如果大小无效，则输出错误信息
        std::cerr << "Invalid packet size: " << packet_data.size() 
                  << " (expected " << PACKAGE_SIZE << ")" << std::endl;
        // 直接返回
        return;
    }
    
    // 调用内部方法处理二进制数据包
    processBinaryPacket(packet_data);
}

// 处理二进制数据包的内部方法
void DataManager::processBinaryPacket(const std::vector<uint8_t>& packet_data) {
    // 使用互斥锁保护数据访问
    std::lock_guard<std::mutex> lock(data_mutex);
    
    // 将字节数据重新解释为浮点数数组
    const float* samples = reinterpret_cast<const float*>(packet_data.data());
    // 计算样本总数
    size_t sample_count = PACKAGE_SIZE / sizeof(float); // 1024 floats
    
    // 遍历每个采样点和通道
    for (size_t sample = 0; sample < SAMPLES_PER_PACKET; ++sample) {
        for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
            // 计算样本在数组中的索引
            size_t index = channel * SAMPLES_PER_PACKET + sample;
            // 确保索引在有效范围内
            if (index < sample_count) {
                // 如果通道缓存已满，则移除最旧的样本
                if (raw_channel_data[channel].size() >= 50000) {
                    raw_channel_data[channel].erase(raw_channel_data[channel].begin());
                }
                // 将新样本添加到对应的原始数据通道
                raw_channel_data[channel].push_back(samples[index]);
            }
        }
        // 更新接收到的总样本数
        total_samples_received++;
    }
}

// 清空所有数据的方法
void DataManager::clear() {
    // 使用互斥锁保护数据访问
    std::lock_guard<std::mutex> data_lock(data_mutex);
    // 使用互斥锁保护显示数据的访问
    std::lock_guard<std::mutex> display_lock(display_mutex);
    
    // 清空缓冲区
    buffer.clear();
    // 清空每个原始数据通道
    for (auto& channel : raw_channel_data) {
        channel.clear();
    }
    // 清空每个显示数据通道
    for (auto& channel : channel_display_data) {
        channel.clear();
    }
    // 清空时间值
    time_values.clear();
    // 重置总样本接收计数器
    total_samples_received = 0;
    // 重置显示样本接收计数器
    display_samples_received = 0;
}

// 获取数据点的方法（当前未使用）
std::vector<DataManager::DataPoint> DataManager::getData() {
    // 使用互斥锁保护数据访问
    std::lock_guard<std::mutex> lock(data_mutex);
    // 返回缓冲区中的数据
    return buffer;
}

// 获取通道显示数据的方法
std::vector<std::vector<float>> DataManager::getChannelDisplayData(size_t max_samples) {
    // 使用互斥锁保护显示数据的访问
    std::lock_guard<std::mutex> lock(display_mutex);
    // 返回通道显示数据
    return channel_display_data;
}

// 获取时间值的方法
std::vector<float> DataManager::getTimeValues() {
    // 使用互斥锁保护显示数据的访问
    std::lock_guard<std::mutex> lock(display_mutex);
    // 返回时间值
    return time_values;
}

// 设置数据处理是否启用的方法
void DataManager::setProcessingEnabled(bool enabled) {
    // 设置处理启用标志
    processing_enabled = enabled;
}

// 检查数据处理是否启用的方法
bool DataManager::isProcessingEnabled() const {
    // 返回处理启用标志
    return processing_enabled;
}

// 新增：设置播放状态的方法
void DataManager::setPlayState(bool playing) {
    // 设置播放状态标志
    is_playing = playing;
}

// 检查是否正在播放的方法
bool DataManager::isPlaying() const {
    // 返回播放状态标志
    return is_playing;
}

// 数据处理线程函数
void DataManager::processData() {
    // 循环直到接收到停止信号
    while (!should_stop) {
        // 如果数据处理已启用
        if (processing_enabled) {
            // 更新显示数据
            updateDisplayData();
        }
        // 线程休眠一段时间，以控制更新频率
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

// 更新显示数据的方法
void DataManager::updateDisplayData() {
    // 使用互斥锁保护数据访问
    std::lock_guard<std::mutex> data_lock(data_mutex);
    // 使用互斥锁保护显示数据的访问
    std::lock_guard<std::mutex> display_lock(display_mutex);
    
    // 只有在播放状态时才更新显示数据
    if (!is_playing) {
        // 如果不是播放状态，则直接返回
        return;
    }
    
    // 如果接收到的总样本数大于已显示的样本数
    if (display_samples_received < total_samples_received) {
        // 更新已显示的样本数
        display_samples_received = total_samples_received;
    }
    
    // 遍历每个通道以更新其显示数据
    for (size_t ch = 0; ch < CHANNEL_COUNT; ++ch) {
        // 如果原始数据通道为空，则跳过
        if (raw_channel_data[ch].empty()) continue;
        
        // 计算要复制的样本数
        size_t samples_to_copy = std::min(raw_channel_data[ch].size(), MAX_DISPLAY_SAMPLES);
        // 计算复制的起始索引
        size_t start_idx = raw_channel_data[ch].size() - samples_to_copy;
        
        // 清空当前通道的显示数据
        channel_display_data[ch].clear();
        // 从原始数据中复制样本到显示数据
        channel_display_data[ch].insert(
            channel_display_data[ch].end(),
            raw_channel_data[ch].begin() + start_idx,
            raw_channel_data[ch].end()
        );
    }
    
    // 更新时间轴，实现滑动窗口效果
    if (!raw_channel_data[0].empty()) {
        // 计算要显示的样本数
        size_t display_samples = std::min(raw_channel_data[0].size(), MAX_DISPLAY_SAMPLES);
        // 计算起始样本点
        size_t start_sample = display_samples_received > display_samples ? 
                              display_samples_received - display_samples : 0;
        
        // 清空时间值
        time_values.clear();
        // 重新计算并填充时间值
        for (size_t i = 0; i < display_samples; ++i) {
            // 根据采样率计算每个样本的时间戳
            double sample_time = (start_sample + i) / SAMPLE_RATE;
            // 将时间戳添加到时间值容器
            time_values.push_back(static_cast<float>(sample_time));
        }
    }
}
