#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <condition_variable>
#include <zmq.h>
#include <algorithm>

namespace fs = std::filesystem;

// 配置参数
constexpr size_t BLOCK_SIZE = 1000;                  // 每次读取的数据包数量
constexpr size_t QUEUE_LOW_WATERMARK = 200;          // 如果低于这个值，触发继续读取
constexpr size_t QUEUE_MAX_SIZE = 5000;              // 最大缓存队列长度

constexpr size_t FS = 22500;                         // 采样率
constexpr size_t POINT_SIZE = 4;                     // 每个采样点的字节数
constexpr size_t POINT_COUNT = 128;                  // 阵元数量
constexpr size_t PACKET_POINT_COUNT = 8;             // 每个数据包的采样点数
// 数据包大小（字节）
constexpr size_t PACKET_BYTE_SIZE = PACKET_POINT_COUNT * POINT_SIZE * POINT_COUNT;
// 每个批次读取的数据缓冲区大小
constexpr size_t READ_CHUNK_SIZE = PACKET_BYTE_SIZE * BLOCK_SIZE;

// 数据发送周期（纳秒）
constexpr int64_t SEND_PERIOD_NS = 1'000'000'000L / (FS / PACKET_POINT_COUNT);

const std::string ADDRESS = "tcp://127.0.0.1:5555";

std::queue<std::vector<uint8_t>> packet_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
std::atomic<bool> running(true);

// 读取线程函数
void read_data_thread(const std::string& folder_path, bool loop_data) {
    while (running) {
        std::vector<fs::path> bin_files;
        for (const auto& entry : fs::directory_iterator(folder_path)) {
            if (entry.path().extension() == ".bin") {
                bin_files.push_back(entry.path());
            }
        }
        std::sort(bin_files.begin(), bin_files.end());

        if (bin_files.empty()) {
            std::cerr << "No .bin files found in " << folder_path << std::endl;
            running = false;
            return;
        }

        for (const auto& file_path : bin_files) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) {
                std::cerr << "Failed to open " << file_path << std::endl;
                continue;
            }

            std::cout << "Reading " << file_path.filename() << std::endl;

            while (running) {
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    queue_cv.wait(lock, [] {
                        return packet_queue.size() < QUEUE_MAX_SIZE || !running;
                    });
                }

                std::vector<uint8_t> buffer(READ_CHUNK_SIZE);
                file.read(reinterpret_cast<char*>(buffer.data()), READ_CHUNK_SIZE);
                size_t bytes_read = file.gcount();

                std::cout << "Read " << bytes_read << " bytes" << std::endl;

                if (bytes_read == 0)
                    break; // 文件结束

                size_t offset = 0;
                while (offset + PACKET_BYTE_SIZE <= bytes_read) {
                    std::vector<uint8_t> packet(buffer.begin() + offset, buffer.begin() + offset + PACKET_BYTE_SIZE);
                    {
                        std::lock_guard<std::mutex> lock(queue_mutex);
                        packet_queue.push(std::move(packet));
                    }
                    offset += PACKET_BYTE_SIZE;
                }

                // 剩余数据不足一个完整包，跳过
            }
        }

        if (!loop_data) {
            running = false;
            break;
        }
    }
    queue_cv.notify_all(); // 通知 sender 线程结束
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <数据文件夹路径> <是否循环:true|[false]>" << std::endl;
        return 1;
    }

    std::string folder_path = argv[1];
    bool loop_data = (argc >= 3) && (std::string(argv[2]) == "true");

    // 初始化 ZMQ
    void* context = zmq_ctx_new();
    if (!context) {
        std::cerr << "Failed to create ZMQ context" << std::endl;
        return -1;
    }

    void* sender = zmq_socket(context, ZMQ_PUSH);
    if (!sender) {
        std::cerr << "Failed to create ZMQ socket" << std::endl;
        zmq_ctx_term(context);
        return -1;
    }

    if (zmq_connect(sender, ADDRESS.c_str()) != 0) {
        std::cerr << "Failed to connect socket: " << zmq_strerror(errno) << std::endl;
        zmq_close(sender);
        zmq_ctx_term(context);
        return -1;
    }

    std::thread reader(read_data_thread, folder_path, loop_data);

    size_t send_count = 0;

    // 发送逻辑在主线程中
    while (running || !packet_queue.empty()) {
        std::vector<uint8_t> packet;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (packet_queue.empty()) {
                queue_cv.wait_for(lock, std::chrono::milliseconds(1));
                continue;
            }

            packet = std::move(packet_queue.front());
            packet_queue.pop();
            queue_cv.notify_one(); // 唤醒读线程补充数据
        }

        int rc = zmq_send(sender, packet.data(), packet.size(), 0);
        if (rc == -1) {
            std::cerr << "Failed to send message: " << zmq_strerror(errno) << std::endl;
            break;
        }

        // 日志输出
        std::cout << "[SEND] Packet #" << (++send_count)
                  << " | Size: " << packet.size() << " bytes" << std::endl;

        std::this_thread::sleep_for(std::chrono::nanoseconds(SEND_PERIOD_NS));
    }

    reader.join();

    zmq_close(sender);
    zmq_ctx_term(context);

    std::cout << "Sender finished. Total packets sent: " << send_count << std::endl;
    return 0;
}
