#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>
#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// 与接收端匹配的常量配置
constexpr size_t CHANNEL_COUNT = 128;           // 通道数
constexpr size_t SAMPLES_PER_PACKET = 8;       // 每个数据包的样本数
constexpr size_t PACKAGE_SIZE = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 4096字节
constexpr double SAMPLE_RATE = 22500.0;        // 采样率
constexpr int TARGET_PORT = 5556;              // 目标端口

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class SpectrogramDataSender {
private:
    int sockfd;
    struct sockaddr_in server_addr;
    float animation_time;
    std::mt19937 rng;
    std::uniform_real_distribution<float> noise_dist;

public:
    SpectrogramDataSender(const std::string& host, int port)
        : sockfd(-1), animation_time(0.0f), rng(std::random_device{}()), noise_dist(0.0f, 1.0f) {

        // 创建socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // 配置服务器地址
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            close(sockfd);
            throw std::runtime_error("Invalid address: " + host);
        }

        // 连接到服务器
        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Connection failed to " + host + ":" + std::to_string(port));
        }

        std::cout << "Connected to " << host << ":" << port << std::endl;
    }

    ~SpectrogramDataSender() {
        if (sockfd >= 0) {
            close(sockfd);
        }
    }

    // 生成模拟频谱数据的函数（基于原始impoltHeartMap.cpp中的生成逻辑）
    void generateSpectrogramData(std::vector<float>& packet_data) {
        packet_data.resize(CHANNEL_COUNT * SAMPLES_PER_PACKET);

        // 为每个通道生成SAMPLES_PER_PACKET个样本
        for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
            double angle_norm = static_cast<double>(channel) / CHANNEL_COUNT;  // 归一化角度值（0-1）
            double angle_deg = angle_norm * 360.0;  // 转换为角度值（0-360度）
            double angle_rad = angle_deg * M_PI / 180.0;  // 转换为弧度值

            for (size_t sample = 0; sample < SAMPLES_PER_PACKET; ++sample) {
                double amplitude = 0.0;

                // 使用动画时间创建移动效果
                double animated_time = animation_time * 0.1 + sample * 0.01;

                // 1. 生成稳定的低角度信号（模拟某个方向的稳定信号源）
                double target_angle1 = 0.1;
                amplitude += 0.5 * exp(-pow(angle_norm - target_angle1, 2) / 0.001);

                // 2. 生成时间变化信号（模拟移动目标）
                double target_angle2 = 0.3 + 0.2 * sin(animated_time * 2 * M_PI);
                amplitude += 0.8 * exp(-pow(angle_norm - target_angle2, 2) / 0.002);

                // 3. 生成旋转扫描信号（模拟雷达扫描）
                double scan_angle = 0.6 + 0.3 * sin(animated_time * M_PI);
                amplitude += 1.0 * exp(-pow(angle_norm - scan_angle, 2) / 0.001);

                // 4. 添加随机噪声
                amplitude += 0.1 * noise_dist(rng);

                // 存储到数据包中
                packet_data[channel * SAMPLES_PER_PACKET + sample] = static_cast<float>(amplitude);
            }
        }
    }

    // 发送数据包
    bool sendPacket(const std::vector<float>& data) {
        if (data.size() != CHANNEL_COUNT * SAMPLES_PER_PACKET) {
            std::cerr << "Invalid packet size: " << data.size() << std::endl;
            return false;
        }

        // 转换为字节数据
        const uint8_t* byte_data = reinterpret_cast<const uint8_t*>(data.data());
        size_t bytes_sent = 0;

        while (bytes_sent < PACKAGE_SIZE) {
            ssize_t result = send(sockfd, byte_data + bytes_sent, PACKAGE_SIZE - bytes_sent, 0);
            if (result < 0) {
                std::cerr << "Send failed: " << strerror(errno) << std::endl;
                return false;
            }
            bytes_sent += result;
        }

        return true;
    }

    // 运行发送循环
    void run() {
        std::cout << "Starting spectrogram data transmission..." << std::endl;
        std::cout << "Channels: " << CHANNEL_COUNT << std::endl;
        std::cout << "Samples per packet: " << SAMPLES_PER_PACKET << std::endl;
        std::cout << "Package size: " << PACKAGE_SIZE << " bytes" << std::endl;
        std::cout << "Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;

        std::vector<float> packet_data;
        auto last_time = std::chrono::high_resolution_clock::now();

        // 计算发送间隔（基于采样率）
        const double interval_ms = (SAMPLES_PER_PACKET * 1000.0) / SAMPLE_RATE;
        const auto sleep_duration = std::chrono::microseconds(static_cast<long>(interval_ms * 1000));

        size_t packet_count = 0;

        while (true) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // 更新动画时间
            animation_time += delta_time;

            // 生成数据
            generateSpectrogramData(packet_data);

            // 发送数据包
            if (!sendPacket(packet_data)) {
                std::cerr << "Failed to send packet " << packet_count << std::endl;
                break;
            }

            packet_count++;
            if (packet_count % 100 == 0) {
                std::cout << "Sent " << packet_count << " packets" << std::endl;
            }

            // 控制发送频率
            std::this_thread::sleep_for(sleep_duration);
        }
    }
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = TARGET_PORT;

    // 解析命令行参数
    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = std::stoi(argv[2]);
    }

    try {
        std::cout << "Spectrogram Data Sender" << std::endl;
        std::cout << "Target: " << host << ":" << port << std::endl;

        SpectrogramDataSender sender(host, port);
        sender.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}