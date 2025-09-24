#include "Core/DataManager.h"  // 包含DataManager类的头文件声明
#include <algorithm>            // 包含标准算法库（如std::clamp等）
#include <chrono>              // 包含时间相关功能（如std::chrono::steady_clock等）

// DataManager类的构造函数 - 初始化数据管理器的所有组件
DataManager::DataManager() {
    m_rawChannelData.resize(CHANNEL_COUNT);        // 调整原始通道数据容器大小，为128个通道分配空间
    m_frontDisplayData.resize(CHANNEL_COUNT);      // 调整前台显示数据容器大小，为128个通道分配空间
    m_backDisplayData.resize(CHANNEL_COUNT);       // 调整后台显示数据容器大小，为128个通道分配空间
    m_processingThread = std::thread(&DataManager::processDataLoop, this); // 创建后台处理线程，执行processDataLoop方法
}

// DataManager类的析构函数 - 清理资源并等待线程结束
DataManager::~DataManager() {
    m_shouldStop = true;                           // 设置停止标志为true，通知后台线程停止运行
    m_queueCv.notify_all();                        // 通知所有等待在条件变量上的线程，解除阻塞
    if (m_processingThread.joinable()) {           // 检查后台线程是否可以join（等待结束）
        m_processingThread.join();                  // 等待后台线程完全结束
    }
}

// 添加二进制数据包的方法 - 由网络接收线程调用，将数据包加入处理队列
void DataManager::addBinaryPacket(const std::vector<uint8_t>& packet_data) {
    if (packet_data.size() != PACKAGE_SIZE) return; // 检查数据包大小是否正确（应为4096字节），不正确则直接返回
    if (!m_isPlaying.load()) return;                 // 检查播放状态，如果暂停则不处理新数据
    {                                                 // 创建作用域来限制锁的范围
        std::lock_guard<std::mutex> lk(m_queueMutex); // 获取队列互斥锁，防止多线程同时访问队列
        m_packetQueue.emplace_back(packet_data);      // 将数据包添加到处理队列末尾
    }                                                 // 作用域结束，自动释放锁
    m_queueCv.notify_one();                          // 通知一个等待的处理线程，有新数据需要处理
}

// 后台数据处理线程的主循环 - 持续处理队列中的数据包
void DataManager::processDataLoop() {
    auto last_update = std::chrono::steady_clock::now(); // 记录上次更新显示数据的时间点
    
    while (!m_shouldStop.load()) {                       // 主循环：直到收到停止信号才退出
        std::vector<uint8_t> packet;                     // 声明数据包变量，用于存储从队列中取出的数据
        {                                                // 创建作用域来限制锁的范围
            std::unique_lock<std::mutex> lk(m_queueMutex); // 获取队列互斥锁，使用unique_lock支持条件变量
            m_queueCv.wait_for(lk, std::chrono::milliseconds(10), [&]{ // 等待队列中有数据，最多等待10毫秒
                return m_shouldStop.load() || !m_packetQueue.empty();   // 等待条件：停止信号或队列非空
            });
            if (m_shouldStop.load()) break;              // 如果收到停止信号，跳出循环
            if (m_packetQueue.empty()) {                 // 如果队列为空，继续下一次循环
                continue;
            }
            packet = std::move(m_packetQueue.front());   // 从队列前端取出数据包，使用移动语义避免拷贝
            m_packetQueue.pop_front();                   // 从队列中移除已取出的数据包
        }                                                // 作用域结束，自动释放锁

        // 解析数据包并写入原始数据缓冲区
        const float* samples = reinterpret_cast<const float*>(packet.data()); // 将uint8数组重新解释为float数组
        const size_t sample_count = PACKAGE_SIZE / sizeof(float);             // 计算数据包中的float样本数量（4096/4=1024）
        for (size_t sample = 0; sample < SAMPLES_PER_PACKET; ++sample) {     // 遍历每个样本（8个样本）
            for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {   // 遍历每个通道（128个通道）
                size_t index = channel * SAMPLES_PER_PACKET + sample;         // 计算在数据包中的索引位置
                if (index < sample_count) {                                   // 确保索引不越界
                    auto& dq = m_rawChannelData[channel];                     // 获取对应通道的原始数据队列引用
                    if (dq.size() >= CHANNEL_HISTORY_SIZE) dq.pop_front();   // 如果队列已满，移除最旧的数据点
                    dq.push_back(samples[index]);                             // 在队列末尾添加新的数据点
                }
            }
            ++m_totalSamplesReceived;                                         // 增加总接收样本计数
        }

        // 按帧率控制更新显示数据
        auto now = std::chrono::steady_clock::now();                          // 获取当前时间点
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update); // 计算距离上次更新的时间间隔

        int target_interval = 1000 / (m_updateFps ? m_updateFps.load() : 1);  // 计算目标更新间隔（毫秒），防止除以0

        if (elapsed.count() >= target_interval) {                             // 如果时间间隔达到目标值
            updateDisplayData();                                              // 调用更新显示数据的方法
            last_update = now;                                                // 更新上次更新的时间点
        }
    }                                                                         // 主循环结束
}

// 更新显示数据的方法 - 将原始数据降采样并准备显示
void DataManager::updateDisplayData() {
    std::string current_chart_type;
    {
        std::lock_guard<std::mutex> lock(m_type_mutex);
        current_chart_type = m_type;
    }
    // 智能降采样：根据目标显示点数动态调整
    size_t target_points = m_targetDisplayPoints.load();                      // 获取目标显示点数
    target_points = std::clamp(target_points, MIN_DISPLAY_POINTS, MAX_DISPLAY_POINTS); // 将目标点数限制在有效范围内
    
    for (size_t ch = 0; ch < CHANNEL_COUNT; ++ch) {                         // 遍历所有通道
        const auto& raw = m_rawChannelData[ch];                              // 获取当前通道的原始数据引用
        auto& out = m_backDisplayData[ch];                                   // 获取当前通道的后台显示数据引用
        out.clear();                                                          // 清空后台显示数据，准备重新填充
        
        if (raw.empty()) continue;                                           // 如果原始数据为空，跳过此通道

        const size_t total_points = raw.size();                              // 获取原始数据的总点数
        if (total_points <= target_points) {                                 // 如果原始点数不超过目标点数
            out.assign(raw.begin(), raw.end());                              // 直接复制所有原始数据
        } else {                                                             // 如果原始点数超过目标点数，需要进行降采样
            // 智能降采样：使用LTTB (Largest-Triangle-Three-Buckets) 类似算法
            size_t bucket_size = total_points / target_points;               // 计算每个桶的大小
            if (bucket_size == 0) bucket_size = 1;                          // 确保桶大小至少为1
            
            // 保留首尾点，中间按桶采样
            out.push_back(raw.front());                                      // 添加第一个数据点
            
            for (size_t i = bucket_size; i < total_points - bucket_size; i += bucket_size) { // 遍历中间的桶
                // 在桶内找最代表性的点（这里简化：取桶中点）
                size_t mid = i + bucket_size / 2;                            // 计算桶中点位置
                if (mid < total_points) {                                    // 确保中点位置有效
                    out.push_back(raw[mid]);                                 // 添加桶中点数据
                }
            }
            
            if (raw.size() > 1) {                                           // 如果原始数据有多个点
                out.push_back(raw.back());                                   // 添加最后一个数据点
            }
            
            // 如果点数还是太多，进一步降采样
            if (out.size() > target_points) {                                // 如果降采样后的点数仍然超过目标
                std::vector<float> temp;                                     // 创建临时向量
                temp.reserve(target_points);                                 // 预分配目标大小的内存
                size_t step = out.size() / target_points;                    // 计算采样步长
                for (size_t i = 0; i < out.size(); i += step) {             // 按步长采样
                    if (temp.size() < target_points) {                      // 如果临时向量未满
                        temp.push_back(out[i]);                             // 添加采样点
                    }
                }
                out.swap(temp);                                             // 交换临时向量和输出向量
            }
        }
    }

    // 更新时间轴（与显示点数对齐的等间隔采样）
    const size_t display_points = m_backDisplayData[0].size();              // 获取显示点数（使用第一个通道作为参考）
    m_backTimeValues.clear();                                               // 清空后台时间值向量
    m_backTimeValues.reserve(display_points);                               // 预分配内存空间

       // 根据图表类型决定X轴的数据
       if (current_chart_type == "putu") {
        // "谱图"模式：X轴为数据点索引 (0, 1, 2, ...)
        for (size_t i = 0; i < display_points; ++i) {
            m_backTimeValues.push_back(static_cast<float>(i));
        }
    } else { // 默认为"shitu" (时图) 或其他任何类型
        // "时图"模式：X轴为真实时间（秒）
        // 估算起始样本索引（确保时间连续）
        const size_t raw_points_count = m_rawChannelData[0].size();     // 获取原始数据点数
        const size_t start_sample_index = m_totalSamplesReceived > raw_points_count ? (m_totalSamplesReceived - raw_points_count) : 0; // 计算起始样本索引

        for (size_t i = 0; i < display_points; ++i) {                    // 遍历每个显示点
            double ratio = display_points > 1 ? static_cast<double>(i) / (display_points - 1) : 0.0; // 计算当前点在显示序列中的比例
            size_t original_index = static_cast<size_t>(ratio * (raw_points_count > 0 ? raw_points_count - 1 : 0)); // 计算对应的原始数据索引
            double t = (start_sample_index + original_index) / SAMPLE_RATE;     // 计算对应的时间值（秒）
            m_backTimeValues.push_back(static_cast<float>(t));              // 将时间值添加到后台时间向量
        }
    }

    // 交换前后缓冲（一次锁）
    {                                                                       // 创建作用域来限制锁的范围
        std::lock_guard<std::mutex> lk(m_displayMutex);                    // 获取显示数据互斥锁
        m_frontDisplayData.swap(m_backDisplayData);                         // 交换前台和后台显示数据
        m_frontTimeValues.swap(m_backTimeValues);                           // 交换前台和后台时间值
    }                                                                       // 作用域结束，自动释放锁
}

// UI线程访问显示数据的接口 - 提供线程安全的数据访问
void DataManager::accessDisplayData(const std::function<void(const std::vector<std::vector<float>>&, const std::vector<float>&)>& accessor) {
    std::lock_guard<std::mutex> lk(m_displayMutex);                        // 获取显示数据互斥锁
    accessor(m_frontDisplayData, m_frontTimeValues);                        // 调用访问者函数，传入前台显示数据和时间值
}

// 清除所有数据的方法 - 清空所有缓冲区和计数器
void DataManager::clear() {
    // 清空所有缓冲
    {                                                                       // 创建作用域来限制锁的范围
        std::lock_guard<std::mutex> qlk(m_queueMutex);                     // 获取队列互斥锁
        m_packetQueue.clear();                                              // 清空数据包队列
    }                                                                       // 作用域结束，自动释放锁
    for (auto& ch : m_rawChannelData) ch.clear();                          // 清空所有通道的原始数据
    {                                                                       // 创建作用域来限制锁的范围
        std::lock_guard<std::mutex> lk(m_displayMutex);                    // 获取显示数据互斥锁
        for (auto& ch : m_frontDisplayData) ch.clear();                    // 清空前台显示数据
        for (auto& ch : m_backDisplayData) ch.clear();                     // 清空后台显示数据
        m_frontTimeValues.clear();                                          // 清空前台时间值
        m_backTimeValues.clear();                                           // 清空后台时间值
    }                                                                       // 作用域结束，自动释放锁
    m_totalSamplesReceived = 0;                                            // 重置总接收样本计数
}

// 设置播放状态的方法
void DataManager::setPlayState(bool playing) { m_isPlaying = playing; }     // 设置播放状态标志

// 获取播放状态的方法
bool DataManager::isPlaying() const { return m_isPlaying.load(); }          // 返回当前播放状态

// 设置更新帧率的方法
void DataManager::setUpdateRate(int fps) { 
    if (fps > 0 && fps <= 120) m_updateFps = fps;                          // 如果帧率在有效范围内，则设置新的帧率
}

// 设置显示点数的方法
void DataManager::setDisplayPoints(size_t points) { 
    m_targetDisplayPoints = std::clamp(points, MIN_DISPLAY_POINTS, MAX_DISPLAY_POINTS); // 将显示点数限制在有效范围内
}
// 设置横轴数据类型
void DataManager::setChartType(const std::string& new_type) {
    // 创建一个 lock_guard，它在构造时自动锁定 m_type_mutex
    std::lock_guard<std::mutex> lock(m_type_mutex);
    
    // 在锁的保护下，安全地修改 m_type
    m_type = new_type;
    
} // 当函数结束时，lock 对象被销毁，自动解锁 m_type_mutex