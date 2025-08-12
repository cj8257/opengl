
#pragma once
#include <mutex>
#include <vector>
#include <atomic>
#include <thread>
#include <cstdint>

struct DataPoint {
    double timestamp;
    double sensor_a;
    double sensor_b;
};

struct ChannelData {
    size_t channel_id;
    double timestamp;
    float value;
};

// 新增：二进制数据包结构
struct BinaryPacket {
    std::vector<uint8_t> data;
    size_t packet_size;
};

class DataManager {
public:
    DataManager();
    ~DataManager();
    
    void addData(const DataPoint& point);
    void addChannelData(const std::vector<std::vector<float>>& channel_samples, double base_timestamp);
    
    // 新增：处理二进制数据包的方法
    void addBinaryPacket(const std::vector<uint8_t>& packet_data);
    void processBinaryPacket(const std::vector<uint8_t>& packet_data);
    
    void clear();
    std::vector<DataPoint> getData();
    std::vector<std::vector<float>> getChannelDisplayData(size_t max_samples = 1000);
    std::vector<float> getTimeValues();
    
    void setProcessingEnabled(bool enabled);
    bool isProcessingEnabled() const;
    
    // 新增：播放控制
    void setPlayState(bool playing);
    bool isPlaying() const;

private:
    void processData();
    void updateDisplayData();
    
    std::vector<DataPoint> buffer;
    std::vector<std::vector<float>> channel_display_data;
    std::vector<float> time_values;
    std::vector<std::vector<float>> raw_channel_data;
    
    std::mutex data_mutex;
    std::mutex display_mutex;
    std::thread processing_thread;
    
    std::atomic<bool> processing_enabled{false};
    std::atomic<bool> should_stop{false};
    std::atomic<bool> is_playing{true};
    
    const size_t maxSize = 1000;
    const size_t CHANNEL_COUNT = 128;
    const size_t SAMPLES_PER_PACKET = 8;
    const size_t MAX_DISPLAY_SAMPLES = 1000;
    const double SAMPLE_RATE = 22500.0; // Hz - 更新为22.5kHz
    const size_t PACKAGE_SIZE = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 4096字节
    
    size_t total_samples_received = 0;
    size_t display_samples_received = 0; // 用于播放控制的显示样本计数
};
