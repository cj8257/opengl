#include "IO/SocketSubscriber.h"  // 包含SocketSubscriber类的头文件声明
#include <iostream>              // 包含标准输入输出流库
#include <sys/socket.h>          // 包含Unix/Linux系统socket系统调用函数
#include <netinet/in.h>          // 包含Internet地址族定义（如sockaddr_in结构）
#include <arpa/inet.h>           // 包含Internet地址转换函数（如inet_addr等）
#include <unistd.h>              // 包含Unix标准函数（如close等）
#include <cstring>               // 包含C字符串函数（如strerror等）
#include <thread>                // 包含C++11线程库
#include <chrono>                // 包含C++11时间库

// SocketSubscriber类的构造函数 - 初始化Socket订阅器
SocketSubscriber::SocketSubscriber(const std::string& host, int port) : host(host), port(port) {} // 初始化列表：设置主机地址和端口号

// SocketSubscriber类的析构函数 - 确保资源清理
SocketSubscriber::~SocketSubscriber() {
    stop(); // 调用stop方法，停止订阅器并释放资源
}

// 启动Socket订阅器的方法 - 开始监听网络连接
void SocketSubscriber::start(BinaryCallback cb) {
    if (running) return; // 如果已经在运行，直接返回，避免重复启动
    binary_callback = cb; // 设置数据接收回调函数，用于处理接收到的二进制数据
    running = true;       // 设置运行标志为true
    worker = std::thread(&SocketSubscriber::run, this); // 启动工作线程，执行run方法
}

// 停止Socket订阅器的方法 - 停止网络监听并清理资源
void SocketSubscriber::stop() {
    if (!running) return; // 如果没有运行，直接返回，避免重复停止
    running = false;      // 设置停止标志为false

    // 关闭socket以解除工作线程的阻塞
    if (client_socket != -1) {            // 如果客户端socket有效（不等于-1）
        shutdown(client_socket, SHUT_RDWR); // 关闭客户端socket的读写功能
        close(client_socket);               // 关闭客户端socket文件描述符
        client_socket = -1;                 // 重置客户端socket为无效值
    }

    if (worker.joinable()) { // 如果工作线程可以join（等待结束）
        worker.join();       // 等待工作线程结束
    }
}

// Socket服务器的主运行逻辑 - 在独立线程中执行
void SocketSubscriber::run() {
    // 创建TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0); // 创建IPv4 TCP socket，返回文件描述符
    if (server_socket == -1) { // 如果创建失败
        std::cerr << "Failed to create socket" << std::endl; // 输出创建socket失败错误信息
        return; // 返回，结束方法执行
    }

    // 允许地址重用 - 避免"地址已在使用"错误
    int opt = 1; // 设置socket选项值
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { // 设置地址重用选项
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl; // 输出设置socket选项失败错误信息
        close(server_socket); // 关闭socket
        return; // 返回，结束方法执行
    }

    // 配置服务器地址结构
    sockaddr_in server_addr; // 声明服务器地址结构体
    server_addr.sin_family = AF_INET;                  // 设置地址族为IPv4
    server_addr.sin_port = htons(port);                // 设置端口号，转换为网络字节序
    server_addr.sin_addr.s_addr = inet_addr(host.c_str()); // 设置IP地址，转换为网络字节序

    // 绑定socket到指定地址和端口
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { // 绑定socket到指定地址和端口
        std::cerr << "Failed to bind socket to " << host << ":" << port << " - " << strerror(errno) << std::endl; // 输出绑定失败错误信息
        close(server_socket); // 关闭socket
        return; // 返回，结束方法执行
    }

    // 开始监听连接请求
    if (listen(server_socket, 1) < 0) { // 开始监听，队列长度为1
        std::cerr << "Failed to listen on socket" << std::endl; // 输出监听失败错误信息
        close(server_socket); // 关闭socket
        return; // 返回，结束方法执行
    }

    std::cout << "SocketSubscriber started, listening on " << host << ":" << port << std::endl; // 输出启动成功信息

    // 定义数据包参数
    const size_t CHANNEL_COUNT = 128;                                   // 通道数量常量：128个通道
    const size_t SAMPLES_PER_PACKET = 8;                               // 每包样本数常量：8个样本
    const size_t PACKAGE_SIZE = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 数据包大小：4字节 * 128通道 * 8样本 = 4096字节

    // 主服务循环 - 持续接受客户端连接并处理数据
    while (running) { // 主循环：直到收到停止信号才退出
        sockaddr_in client_addr;    // 声明客户端地址结构体
        socklen_t client_len = sizeof(client_addr); // 客户端地址结构体长度
        // 接受客户端连接
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len); // 接受客户端连接，返回客户端socket描述符

        if (client_socket < 0) { // 如果接受连接失败
            if (!running) break; // 如果收到停止信号，退出循环
            std::cerr << "Failed to accept connection" << std::endl; // 输出接受连接失败错误信息
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 短暂休眠100毫秒
            continue; // 继续下一次循环
        }

        // 获取并显示客户端IP地址
        char client_ip[INET_ADDRSTRLEN]; // 声明客户端IP地址字符串缓冲区
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN); // 将网络字节序IP地址转换为字符串
        std::cout << "Client connected from " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl; // 输出客户端连接信息

        // 创建接收缓冲区
        std::vector<uint8_t> buffer(PACKAGE_SIZE); // 创建数据包大小的接收缓冲区
        // 客户端数据接收循环
        while (running) { // 内层循环：处理单个客户端的数据
            ssize_t total_bytes_read = 0; // 已读取的总字节数
            // 确保读取完整的数据包
            while (total_bytes_read < PACKAGE_SIZE) { // 循环读取，直到读取完整的数据包
                // 接收数据到缓冲区
                ssize_t bytes_read = recv(client_socket, buffer.data() + total_bytes_read, PACKAGE_SIZE - total_bytes_read, 0); // 接收数据
                if (bytes_read > 0) { // 如果成功读取到数据
                    total_bytes_read += bytes_read; // 累加已读取字节数
                } else if (bytes_read == 0) { // 如果客户端关闭连接
                    // 客户端关闭连接
                    std::cout << "Client disconnected." << std::endl; // 输出客户端断开连接信息
                    break; // 退出内层循环
                } else { // 如果接收出错
                    // 接收错误
                    if (errno != EWOULDBLOCK && errno != EAGAIN) { // 如果不是非阻塞错误
                        std::cerr << "Recv error: " << strerror(errno) << std::endl; // 输出接收错误信息
                        break; // 退出内层循环
                    }
                    // 当前没有数据可用，短暂休眠
                    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 休眠1毫秒
                }
            }

            // 检查是否读取了完整的数据包
            if (total_bytes_read == PACKAGE_SIZE) { // 如果读取了完整的数据包
                if (binary_callback) {  // 如果设置了回调函数
                    binary_callback(buffer); // 调用回调函数处理数据
                }
            } else { // 如果数据包不完整
                // 连接被关闭或发生错误
                break; // 退出外层循环
            }
        }

        // 关闭客户端连接
        if (client_socket != -1) { // 如果客户端socket有效
            close(client_socket); // 关闭客户端socket
            client_socket = -1;   // 重置为无效值
        }
    } // 主服务循环结束

    // 清理服务器socket
    if (server_socket != -1) { // 如果服务器socket有效
        close(server_socket); // 关闭服务器socket
        server_socket = -1;   // 重置为无效值
    }
    std::cout << "SocketSubscriber stopped" << std::endl; // 输出停止信息
}