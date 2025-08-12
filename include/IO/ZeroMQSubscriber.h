
#pragma once
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>

class ZeroMQSubscriber {
public:
    // 更新回调函数以支持二进制数据
    using BinaryCallback = std::function<void(const std::vector<uint8_t>&)>;
    using StringCallback = std::function<void(const std::string&)>; // 保留向后兼容性

    ZeroMQSubscriber(const std::string& endpoint);
    ~ZeroMQSubscriber();

    // 新增：二进制数据回调
    void start(BinaryCallback cb);
    // 保留原有的字符串回调（向后兼容）
    void startString(StringCallback cb);
    
    void stop();

private:
    void run();
    void runString();
    
    std::string endpoint;
    BinaryCallback binary_callback;
    StringCallback string_callback;
    std::thread worker;
    std::atomic<bool> running{false};
    std::atomic<bool> use_binary_mode{true}; // 默认使用二进制模式
};
