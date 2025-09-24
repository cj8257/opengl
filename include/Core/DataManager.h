#pragma once

#include <vector>
#include <deque>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <cstdint>
#include <functional>
#include <condition_variable>

class DataManager {
public:
    DataManager();
    ~DataManager();

    // 网络线程调用：仅入队数据包（非常轻量）
    void addBinaryPacket(const std::vector<uint8_t>& packet_data);

    // UI线程调用：安全访问前台显示数据（连续内存）
    void accessDisplayData(const std::function<void(const std::vector<std::vector<float>>& data, const std::vector<float>& time)>& accessor);

    // 控制接口
    void clear();
    void setPlayState(bool playing);
    bool isPlaying() const;
    
    // 显示参数控制
    void setUpdateRate(int fps);
    void setDisplayPoints(size_t points);
    void setChartType(const std::string& new_type);
private:
    // 处理线程主循环
    void processDataLoop();
    // 根据原始缓冲生成降采样显示数据并与前台交换
    void updateDisplayData();

private:
    // 常量配置（与发送端/订阅端保持一致）
    static constexpr size_t CHANNEL_COUNT       = 128;
    static constexpr size_t SAMPLES_PER_PACKET  = 8;
    static constexpr double SAMPLE_RATE         = 22500.0;
    static constexpr size_t PACKAGE_SIZE        = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 4096

    // 历史原始数据容量（每通道最多保留的样本数）
    static constexpr size_t CHANNEL_HISTORY_SIZE = 20000;
    // 显示点数范围（降采样后每通道显示的点数范围）
    static constexpr size_t MIN_DISPLAY_POINTS   = 100;
    static constexpr size_t MAX_DISPLAY_POINTS   = 2000;

    // 原始数据（仅处理线程写入/读取）
    std::vector<std::deque<float>> m_rawChannelData;
    uint64_t m_totalSamplesReceived = 0; // 用于计算时间轴

    // 数据包队列（接收线程->处理线程）
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
    std::deque<std::vector<uint8_t>> m_packetQueue;

    // 前后显示缓冲（UI线程读，处理线程写）
    std::mutex m_displayMutex;
    std::vector<std::vector<float>> m_frontDisplayData;
    std::vector<std::vector<float>> m_backDisplayData;
    std::vector<float> m_frontTimeValues;
    std::vector<float> m_backTimeValues;

    // 控制与线程
    std::atomic<bool> m_isPlaying{true};
    std::atomic<bool> m_shouldStop{false};
    std::thread m_processingThread;
    
    // 显示控制参数
    std::atomic<int> m_updateFps{30};          // 目标更新帧率
    std::atomic<size_t> m_targetDisplayPoints{1000}; // 目标显示点数
    std::string m_type{""}; // 图表类型。注意：空字符串是 "" 而不是 ''
    mutable std::mutex m_type_mutex; // 用于保护 m_type 的互斥锁
};