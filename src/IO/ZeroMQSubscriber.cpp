
#include "IO/ZeroMQSubscriber.h"
#include <zmq.h>
#include <iostream>
#include <thread>
#include <chrono>

ZeroMQSubscriber::ZeroMQSubscriber(const std::string& endpoint) : endpoint(endpoint) {}

ZeroMQSubscriber::~ZeroMQSubscriber() {
    stop();
}

// 新增：二进制数据模式启动
void ZeroMQSubscriber::start(BinaryCallback cb) {
    if (running) return;
    binary_callback = cb;
    use_binary_mode = true;
    running = true;
    worker = std::thread(&ZeroMQSubscriber::run, this);
}

// 保留原有的字符串模式（向后兼容）
void ZeroMQSubscriber::startString(StringCallback cb) {
    if (running) return;
    string_callback = cb;
    use_binary_mode = false;
    running = true;
    worker = std::thread(&ZeroMQSubscriber::runString, this);
}

void ZeroMQSubscriber::stop() {
    if (!running) return;
    running = false;
    if (worker.joinable()) worker.join();
}

// 二进制数据接收模式 - 使用C API以获得更好的控制
void ZeroMQSubscriber::run() {
    void* context = zmq_ctx_new();
    if (!context) {
        std::cerr << "Failed to create ZMQ context" << std::endl;
        return;
    }
    
    void* receiver = zmq_socket(context, ZMQ_PULL);
    if (!receiver) {
        std::cerr << "Failed to create ZMQ socket" << std::endl;
        zmq_ctx_destroy(context);
        return;
    }
    
    // 设置接收超时
    int timeout = 100; // 100ms
    zmq_setsockopt(receiver, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 绑定到端点（与main.cpp一致）
    if (zmq_bind(receiver, endpoint.c_str()) != 0) {
        std::cerr << "Failed to bind socket: " << zmq_strerror(errno) << std::endl;
        zmq_close(receiver);
        zmq_ctx_destroy(context);
        return;
    }

    // 数据包大小常量（与main.cpp和DataManager一致）
    const size_t CHANNEL_COUNT = 128;
    const size_t SAMPLES_PER_PACKET = 8;
    const size_t PACKAGE_SIZE = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 4096字节
    
    std::cout << "ZeroMQSubscriber started in binary mode, listening on " << endpoint << std::endl;

    while (running) {
        std::vector<uint8_t> buffer(PACKAGE_SIZE);
        int recv_size = zmq_recv(receiver, buffer.data(), buffer.size(), ZMQ_DONTWAIT);
        
        if (recv_size > 0) {
            // 检查数据包大小
            if (recv_size == static_cast<int>(PACKAGE_SIZE)) {
                buffer.resize(recv_size); // 确保大小正确
                if (binary_callback) {
                    binary_callback(buffer);
                }
            } else {
                std::cerr << "Received unexpected packet size: " << recv_size 
                          << " (expected " << PACKAGE_SIZE << ")" << std::endl;
            }
        } else if (recv_size == -1 && errno != EAGAIN) {
            // 只有非超时错误才输出
            std::cerr << "ZMQ recv error: " << zmq_strerror(errno) << std::endl;
        }
        
        // 让出CPU时间
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    zmq_close(receiver);
    zmq_ctx_destroy(context);
    std::cout << "ZeroMQSubscriber stopped" << std::endl;
}

// 字符串数据接收模式（保留向后兼容）
void ZeroMQSubscriber::runString() {
    void* context = zmq_ctx_new();
    if (!context) {
        std::cerr << "Failed to create ZMQ context" << std::endl;
        return;
    }
    
    void* socket = zmq_socket(context, ZMQ_SUB);
    if (!socket) {
        std::cerr << "Failed to create ZMQ socket" << std::endl;
        zmq_ctx_destroy(context);
        return;
    }
    
    // 订阅所有消息
    zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);
    
    if (zmq_connect(socket, endpoint.c_str()) != 0) {
        std::cerr << "Failed to connect socket: " << zmq_strerror(errno) << std::endl;
        zmq_close(socket);
        zmq_ctx_destroy(context);
        return;
    }

    while (running) {
        char buffer[1024];
        int recv_size = zmq_recv(socket, buffer, sizeof(buffer) - 1, ZMQ_DONTWAIT);
        
        if (recv_size > 0) {
            buffer[recv_size] = '\0'; // null terminate
            std::string json(buffer);
            if (string_callback) {
                string_callback(json);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    zmq_close(socket);
    zmq_ctx_destroy(context);
}
