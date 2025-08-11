// receiver.cpp
#include <zmq.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <csignal>
#include <fstream>
#include <cstdint>
#include <atomic>

const int PORT                       = 5555;
const std::string ADDRESS            = "tcp://0.0.0.0:" + std::to_string(PORT);
const size_t CHANNEL_COUNT           = 128; // 阵元数量
const size_t SAMPLES_PER_PACKET      = 8; // 每个包的样本数量
const size_t package_size            = 4 * CHANNEL_COUNT * SAMPLES_PER_PACKET; // 和 sender 保持一致
const size_t MAX_SAMPLES_PER_CHANNEL = 10000;

std::atomic<bool> running(true);
// Cache for channel samples
std::vector<std::vector<float>> channel_samples(CHANNEL_COUNT);

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        std::cout << "Received termination signal. Exiting..." << std::endl;
        running = false;
    }
}

// Save cached samples to a binary file
void save_samples_to_file(const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return;
    }

    for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
        const auto& samples = channel_samples[channel];
        // Write channel index (optional, for identification)
        out.write(reinterpret_cast<const char*>(&channel), sizeof(channel));
        // Write number of samples
        size_t sample_count = samples.size();
        out.write(reinterpret_cast<const char*>(&sample_count), sizeof(sample_count));
        // Write samples
        out.write(reinterpret_cast<const char*>(samples.data()), sample_count * sizeof(float));
    }
    std::cout << "Saved cached samples to " << filename << std::endl;
}

int main(int argc, char* argv[]) {

    // 注册信号处理函数
    std::signal(SIGINT, signal_handler);

    // Initialize channel samples cache
    for (auto& samples : channel_samples) {
        samples.reserve(1000); // Pre-allocate to reduce reallocations
    }

    // 初始化 ZMQ 上下文
    void* context = zmq_ctx_new();
    if (!context) {
        std::cerr << "Failed to create ZMQ context" << std::endl;
        return -1;
    }

    // 创建 PULL 类型的套接字
    void* receiver = zmq_socket(context, ZMQ_PULL);
    if (!receiver) {
        std::cerr << "Failed to create ZMQ socket" << std::endl;
        zmq_ctx_destroy(context);
        return -1;
    }

    // Set receive high-water mark
    int hwm = 1000;
    zmq_setsockopt(receiver, ZMQ_RCVHWM, &hwm, sizeof(hwm));

    // 监听端口
    if (zmq_bind(receiver, ADDRESS.c_str()) != 0) {
        std::cerr << "Failed to bind socket : " << zmq_strerror(errno)  << std::endl;
        zmq_close(receiver);
        zmq_ctx_destroy(context);
        return -1;
    }

    std::cout << "Receiver started, Listening on: " << ADDRESS << std::endl;

    // 接收循环
    int count = 0;
    std::vector<uint8_t> buffer(package_size); // 假设 package_size 是一个宏定义的缓冲区大小

    while (true) {
        int recv_size = zmq_recv(receiver, buffer.data(), buffer.size(), 0);
        if (recv_size == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            std::cerr << "Failed to receive message: " << zmq_strerror(errno) << std::endl;
            break;
        }

        if (recv_size != static_cast<int>(package_size)) {
            std::cerr << "Received unexpected packet size: " << recv_size << " (expected " << package_size << ")" << std::endl;
            continue;
        }

        // Decode the buffer
        const float* samples = reinterpret_cast<const float*>(buffer.data());
        size_t sample_count = package_size / sizeof(float); // 128 * 8 = 1024 floats

        // Cache samples for each channel
        for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
            if (channel_samples[channel].size() >= MAX_SAMPLES_PER_CHANNEL) {
                std::cerr << "Channel " << channel << " cache full, discarding packet" << std::endl;
                continue;
            }
            for (size_t sample = 0; sample < SAMPLES_PER_PACKET; ++sample) {
                size_t index = channel * SAMPLES_PER_PACKET + sample;
                if (index < sample_count) {
                    channel_samples[channel].push_back(samples[index]);
                }
            }
        }

        std::cout << "[RECV] #" << (++count) << " | Size: " << recv_size << " bytes" << std::endl;

        // Optional: Print a few samples from the first channel for debugging
        if (!channel_samples[0].empty()) {
            std::cout << "Channel 0 (last " << SAMPLES_PER_PACKET << " samples): ";
            size_t start = channel_samples[0].size() > SAMPLES_PER_PACKET ? channel_samples[0].size() - SAMPLES_PER_PACKET : 0;
            for (size_t i = start; i < channel_samples[0].size(); ++i) {
                std::cout << channel_samples[0][i] << " ";
            }
            std::cout << std::endl;
        }
    }
    
    // Save cached samples to file before exiting
    save_samples_to_file("cached_samples.bin");

    // Print cache summary
    std::cout << "Cache summary:" << std::endl;
    for (size_t channel = 0; channel < CHANNEL_COUNT; ++channel) {
        std::cout << "Channel " << channel << ": " << channel_samples[channel].size() << " samples" << std::endl;
    }

    zmq_close(receiver);
    zmq_ctx_destroy(context);
    std::cout << "Receiver finished. Total packets received: " << count << std::endl;

    return 0;
}
