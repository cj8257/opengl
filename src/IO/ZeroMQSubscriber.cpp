
#include "IO/ZeroMQSubscriber.h"  // 包含ZeroMQ订阅器头文件
#include <zmq.hpp>  // 包含ZeroMQ C++库头文件

ZeroMQSubscriber::ZeroMQSubscriber(const std::string& endpoint) : endpoint(endpoint) {}  // 构造函数，初始化端点地址

ZeroMQSubscriber::~ZeroMQSubscriber() {  // 析构函数
    stop();  // 停止订阅服务
}

void ZeroMQSubscriber::start(Callback cb) {  // 启动订阅服务的方法
    if (running) return;  // 如果已经在运行则直接返回
    callback = cb;  // 保存回调函数
    running = true;  // 设置运行状态为true
    worker = std::thread(&ZeroMQSubscriber::run, this);  // 创建工作线程
}

void ZeroMQSubscriber::stop() {  // 停止订阅服务的方法
    if (!running) return;  // 如果未运行则直接返回
    running = false;  // 设置运行状态为false
    if (worker.joinable()) worker.join();  // 等待工作线程结束
}

void ZeroMQSubscriber::run() {  // 工作线程的运行函数
    zmq::context_t context(1);  // 创建ZeroMQ上下文
    zmq::socket_t socket(context, zmq::socket_type::sub);  // 创建订阅类型套接字
    socket.connect(endpoint);  // 连接到指定端点
    socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);  // 订阅所有消息

    while (running) {  // 当运行状态为true时循环
        zmq::message_t msg;  // 创建消息对象
        if (socket.recv(msg, zmq::recv_flags::none)) {  // 接收消息
            std::string json(static_cast<char*>(msg.data()), msg.size());  // 将消息转换为JSON字符串
            if (callback) callback(json);  // 调用回调函数处理JSON数据
        }
    }
}
