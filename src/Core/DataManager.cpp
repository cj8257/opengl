
#include "Core/DataManager.h"
#include <algorithm>
#include <chrono>
#include <iostream>

DataManager::DataManager() {
    raw_channel_data.resize(CHANNEL_COUNT);
    channel_display_data.resize(CHANNEL_COUNT);
    
    for (auto& channel : raw_channel_data) {
        channel.reserve(10000);
    }
    
    for (auto& channel : channel_display_data) {
        channel.reserve(MAX_DISPLAY_SAMPLES);
    }
    
    time_values.reserve(MAX_DISPLAY_SAMPLES);
    
    processing_thread = std::thread(&DataManager::processData, this);
}

DataManager::~DataManager() {
    should_stop = true;
    if (processing_thread.joinable()) {
        processing_thread.join();
    }
}

void DataManager::addData(const DataPoint& point) {
    std::lock_guard<std::mutex> lock(data_mutex);
    if (buffer.size() >= maxSize)
        buffer.erase(buffer.begin());
    buffer.push_back(point);
}

void DataManager::addChannelData(const std::vector<std::vector<float>>& channel_samples, double base_timestamp) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    if (channel_samples.size() != CHANNEL_COUNT) return;
    
    for (size_t ch = 0; ch < CHANNEL_COUNT; ++ch) {
        const auto& samples = channel_samples[ch];
        for (const float sample : samples) {
            if (raw_channel_data[ch].size() >= 50000) {
                raw_channel_data[ch].erase(raw_channel_data[ch].begin());
            }
            raw_channel_data[ch].push_back(sample);
        }
    }
    
    total_samples_received += (channel_samples.empty() ? 0 : channel_samples[0].size());
}

// 新增：处理二进制数据包
void DataManager::addBinaryPacket(const std::vector<uint8_t>& packet_data) {
    if (packet_data.size() != PACKAGE_SIZE) {
        std::cerr << "Invalid packet size: " << packet_data.size() 
                  << " (expected " << PACKAGE_SIZE << ")" << std::endl;
        return;
    }
    
    processBinaryPacket(packet_data);
}

void DataManager::processBinaryPacket(const std::vector<uint8_t>& packet_data) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    // 将字节数据转换为浮点数
    const float* samples = reinterpret_cast<const float*>(packet_data.data());
    size_t sample_count = PACKAGE_SIZE / sizeof(float); // 1024 floats
    
    // 按照main.cpp的方式处理数据：遍历每个采样点和通道
    for (size_t sample = 0; sample < SAMPLES_PER_PACKET; ++sample) {
        for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
            size_t index = channel * SAMPLES_PER_PACKET + sample;
            if (index < sample_count) {
                // 保持通道缓存大小限制
                if (raw_channel_data[channel].size() >= 50000) {
                    raw_channel_data[channel].erase(raw_channel_data[channel].begin());
                }
                raw_channel_data[channel].push_back(samples[index]);
            }
        }
        total_samples_received++;
    }
}

void DataManager::clear() {
    std::lock_guard<std::mutex> data_lock(data_mutex);
    std::lock_guard<std::mutex> display_lock(display_mutex);
    
    buffer.clear();
    for (auto& channel : raw_channel_data) {
        channel.clear();
    }
    for (auto& channel : channel_display_data) {
        channel.clear();
    }
    time_values.clear();
    total_samples_received = 0;
    display_samples_received = 0;
}

std::vector<DataPoint> DataManager::getData() {
    std::lock_guard<std::mutex> lock(data_mutex);
    return buffer;
}

std::vector<std::vector<float>> DataManager::getChannelDisplayData(size_t max_samples) {
    std::lock_guard<std::mutex> lock(display_mutex);
    return channel_display_data;
}

std::vector<float> DataManager::getTimeValues() {
    std::lock_guard<std::mutex> lock(display_mutex);
    return time_values;
}

void DataManager::setProcessingEnabled(bool enabled) {
    processing_enabled = enabled;
}

bool DataManager::isProcessingEnabled() const {
    return processing_enabled;
}

// 新增：播放控制方法
void DataManager::setPlayState(bool playing) {
    is_playing = playing;
}

bool DataManager::isPlaying() const {
    return is_playing;
}

void DataManager::processData() {
    while (!should_stop) {
        if (processing_enabled) {
            updateDisplayData();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

void DataManager::updateDisplayData() {
    std::lock_guard<std::mutex> data_lock(data_mutex);
    std::lock_guard<std::mutex> display_lock(display_mutex);
    
    // 只有在播放状态时才更新显示数据
    if (!is_playing) {
        return;
    }
    
    // 更新显示样本计数
    if (display_samples_received < total_samples_received) {
        display_samples_received = total_samples_received;
    }
    
    // 更新每个通道的显示数据
    for (size_t ch = 0; ch < CHANNEL_COUNT; ++ch) {
        if (raw_channel_data[ch].empty()) continue;
        
        size_t samples_to_copy = std::min(raw_channel_data[ch].size(), MAX_DISPLAY_SAMPLES);
        size_t start_idx = raw_channel_data[ch].size() - samples_to_copy;
        
        channel_display_data[ch].clear();
        channel_display_data[ch].insert(
            channel_display_data[ch].end(),
            raw_channel_data[ch].begin() + start_idx,
            raw_channel_data[ch].end()
        );
    }
    
    // 更新时间轴 - 实现滑动窗口
    if (!raw_channel_data[0].empty()) {
        size_t display_samples = std::min(raw_channel_data[0].size(), MAX_DISPLAY_SAMPLES);
        size_t start_sample = display_samples_received > display_samples ? 
                              display_samples_received - display_samples : 0;
        
        time_values.clear();
        for (size_t i = 0; i < display_samples; ++i) {
            double sample_time = (start_sample + i) / SAMPLE_RATE;
            time_values.push_back(static_cast<float>(sample_time));
        }
    }
}
