// 包含ZeroMQSubscriber的头文件
#include "IO/ZeroMQSubscriber.h"
// 包含ZeroMQ库的头文件
#include <zmq.h>
// 包含输入输出流库的头文件
#include <iostream>
// 包含线程库的头文件
#include <thread>
// 包含chrono库的头文件，用于时间相关的操作
#include <chrono>

// ZeroMQSubscriber类的构造函数，初始化端点
ZeroMQSubscriber::ZeroMQSubscriber(const std::string& endpoint) : endpoint(endpoint) {}

// ZeroMQSubscriber类的析构函数
ZeroMQSubscriber::~ZeroMQSubscriber() {
    // 调用stop方法停止订阅
    stop();
}

// 新增：以二进制模式启动订阅
void ZeroMQSubscriber::start(BinaryCallback cb) {
    // 如果已经在运行，则直接返回
    if (running) return;
    // 设置二进制数据回调函数
    binary_callback = cb;
    // 设置为二进制模式
    use_binary_mode = true;
    // 设置运行状态为true
    running = true;
    // 创建并启动一个新的工作线程来运行run方法
    worker = std::thread(&ZeroMQSubscriber::run, this);
}

// 保留原有的字符串模式（向后兼容）
void ZeroMQSubscriber::startString(StringCallback cb) {
    // 如果已经在运行，则直接返回
    if (running) return;
    // 设置字符串数据回调函数
    string_callback = cb;
    // 设置为非二进制模式
    use_binary_mode = false;
    // 设置运行状态为true
    running = true;
    // 创建并启动一个新的工作线程来运行runString方法
    worker = std::thread(&ZeroMQSubscriber::runString, this);
}

// 停止订阅的方法
void ZeroMQSubscriber::stop() {
    // 如果没有在运行，则直接返回
    if (!running) return;
    // 设置运行状态为false
    running = false;
    // 如果工作线程是可加入的，则等待其执行完毕
    if (worker.joinable()) worker.join();
}

// 二进制数据接收模式的运行方法
void ZeroMQSubscriber::run() {
    // 创建一个新的ZMQ上下文
    void* context = zmq_ctx_new();
    // 检查上下文是否创建成功
    if (!context) {
        // 如果失败，则输出错误信息
        std::cerr << "Failed to create ZMQ context" << std::endl;
        // 直接返回
        return;
    }
    
    // 创建一个ZMQ_PULL类型的套接字
    void* receiver = zmq_socket(context, ZMQ_PULL);
    // 检查套接字是否创建成功
    if (!receiver) {
        // 如果失败，则输出错误信息
        std::cerr << "Failed to create ZMQ socket" << std::endl;
        // 销毁ZMQ上下文
        zmq_ctx_destroy(context);
        // 直接返回
        return;
    }
    
    // 设置接收超时时间
    int timeout = 100; // 100ms
    // 将超时选项设置到套接字上
    zmq_setsockopt(receiver, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    
    // 将套接字绑定到指定的端点
    if (zmq_bind(receiver, endpoint.c_str()) != 0) {
        // 如果绑定失败，则输出错误信息
        std::cerr << "Failed to bind socket: " << zmq_strerror(errno) << std::endl;
        // 关闭套接字
        zmq_close(receiver);
        // 销毁ZMQ上下文
        zmq_ctx_destroy(context);
        // 直接返回
        return;
    }

    // 定义数据包大小相关的常量
    const size_t CHANNEL_COUNT = 128;
    const size_t SAMPLES_PER_PACKET = 8;
    const size_t PACKAGE_SIZE = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 4096字节
    
    // 输出启动信息
    std::cout << "ZeroMQSubscriber started in binary mode, listening on " << endpoint << std::endl;

    // 循环直到接收到停止信号
    while (running) {
        // 创建一个用于接收数据的缓冲区
        std::vector<uint8_t> buffer(PACKAGE_SIZE);
        // 以非阻塞方式接收数据
        int recv_size = zmq_recv(receiver, buffer.data(), buffer.size(), ZMQ_DONTWAIT);
        
        // 如果成功接收到数据
        if (recv_size > 0) {
            // 检查接收到的数据包大小是否符合预期
            if (recv_size == static_cast<int>(PACKAGE_SIZE)) {
                // 调整缓冲区大小以匹配实际接收的大小
                buffer.resize(recv_size);
                // 如果设置了二进制回调函数
                if (binary_callback) {
                    // 调用回调函数处理接收到的数据
                    binary_callback(buffer);
                }
            } else {
                // 如果数据包大小不符合预期，则输出错误信息
                std::cerr << "Received unexpected packet size: " << recv_size 
                          << " (expected " << PACKAGE_SIZE << ")" << std::endl;
            }
        } else if (recv_size == -1 && errno != EAGAIN) {
            // 如果接收出错且不是因为资源暂时不可用（超时）
            // 输出ZMQ接收错误信息
            std::cerr << "ZMQ recv error: " << zmq_strerror(errno) << std::endl;
        }
        
        // 让出CPU时间，避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // 关闭套接字
    zmq_close(receiver);
    // 销毁ZMQ上下文
    zmq_ctx_destroy(context);
    // 输出停止信息
    std::cout << "ZeroMQSubscriber stopped" << std::endl;
}

// 字符串数据接收模式的运行方法（保留向后兼容）
void ZeroMQSubscriber::runString() {
    // 创建一个新的ZMQ上下文
    void* context = zmq_ctx_new();
    // 检查上下文是否创建成功
    if (!context) {
        // 如果失败，则输出错误信息
        std::cerr << "Failed to create ZMQ context" << std::endl;
        // 直接返回
        return;
    }
    
    // 创建一个ZMQ_SUB类型的套接字
    void* socket = zmq_socket(context, ZMQ_SUB);
    // 检查套接字是否创建成功
    if (!socket) {
        // 如果失败，则输出错误信息
        std::cerr << "Failed to create ZMQ socket" << std::endl;
        // 销毁ZMQ上下文
        zmq_ctx_destroy(context);
        // 直接返回
        return;
    }
    
    // 设置套接字选项，订阅所有消息
    zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);
    
    // 连接到指定的端点
    if (zmq_connect(socket, endpoint.c_str()) != 0) {
        // 如果连接失败，则输出错误信息
        std::cerr << "Failed to connect socket: " << zmq_strerror(errno) << std::endl;
        // 关闭套接字
        zmq_close(socket);
        // 销毁ZMQ上下文
        zmq_ctx_destroy(context);
        // 直接返回
        return;
    }

    // 循环直到接收到停止信号
    while (running) {
        // 创建一个用于接收数据的缓冲区
        char buffer[1024];
        // 以非阻塞方式接收数据
        int recv_size = zmq_recv(socket, buffer, sizeof(buffer) - 1, ZMQ_DONTWAIT);
        
        // 如果成功接收到数据
        if (recv_size > 0) {
            // 在接收到的数据末尾添加空字符，以形成一个有效的C字符串
            buffer[recv_size] = '\0';
            // 将C字符串转换为std::string
            std::string json(buffer);
            // 如果设置了字符串回调函数
            if (string_callback) {
                // 调用回调函数处理接收到的JSON字符串
                string_callback(json);
            }
        }
        
        // 让出CPU时间，避免忙等待
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // 关闭套接字
    zmq_close(socket);
    // 销毁ZMQ上下文
    zmq_ctx_destroy(context);
}
